#include "ranks.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "file_util.h"

void free_symbol_to_ranks_hash_map(SymbolToRanksHashMap* ranks) {
    for (size_t i = 0; i < RANK_MAP_SIZE; ++i) {
        StockRanks* sr = ranks->symbol_to_ranks[i];
        if (sr) {
            free(sr->rank_per_day);
            free(sr->low_per_day);
            free(sr->high_per_day);
            free(sr->dates);
            free(sr->rank_diffs);
            free(sr->went_up);
            free(sr);
        }
    }
    free(ranks);
}

/**
 * Fills each rank's rank_diffs and went_up.
 *
 * @param symbol_to_ranks_map The rank's rank_per_day, low_per_day, and
 *                            high_per_day must contain valid data
 *                            for this function to work.
 * @param days_per_diff Number of days to look back when computing rank diffs.
 * @param buy_sell_lag The number of days after a diff to query the price
 *                     for a rank's went_up.
 */
void create_rank_diffs(
    const SymbolToRanksHashMap* symbol_to_ranks_map,
    const size_t days_per_diff,
    const size_t buy_sell_lag
) {
    for (size_t j = 0; j < RANK_MAP_SIZE; j++) {
        const StockRanks* stock_ranks = symbol_to_ranks_map->symbol_to_ranks[j];
        if (stock_ranks == NULL ||
            // (the diff size) + (the day after_diff) + (the price lag)
            stock_ranks->data_size < (days_per_diff + 1) + (1) + (buy_sell_lag)
        ) {
            continue;
        }
        for (size_t i = days_per_diff;
             // (the data size) - (the day after a diff) - (the price lag)
             i < (stock_ranks->data_size) - (1) - (buy_sell_lag);
             i++
        ) {
            stock_ranks->rank_diffs[i] =
                stock_ranks->rank_per_day[i] -
                stock_ranks->rank_per_day[i - days_per_diff];

            assert(i + 1 + buy_sell_lag < stock_ranks->data_size);

            // the day after diff, if bought, assume worst case of high price
            const double high_day_after_diff = stock_ranks->high_per_day[i + 1];
            // after the number of lag days, if sold, assume worst case of low price
            const double low_buy_or_sell =
                stock_ranks->low_per_day[i + 1 + buy_sell_lag];
            // stored with the day the diff ends so indices line up later
            stock_ranks->went_up[i] = low_buy_or_sell > high_day_after_diff;
        }
    }
}

/**
 * Checks if the given year is a leap year.
 *
 * @param year The year to check.
 * @return true if given year is a leap year, else false.
 */
bool is_leap_year(const uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/**
 * Increments the given date.
 *
 * @param date The date to increment to the next date.
 */
void next_day(Date* date) {
    static const uint8_t days_in_month[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    const uint16_t year = date->year;
    const uint8_t month = date->month;
    const uint8_t day = date->day;
    uint8_t days_in_current_month = days_in_month[month - 1];
    if (month == 2 && is_leap_year(year)) {
        days_in_current_month = 29;
    }

    if (day < days_in_current_month) {
        date->day++;
    } else {
        date->day = 1;
        if (month < 12) {
            date->month++;
        } else {
            date->month = 1;
            date->year++;
        }
    }
}

/**
 * Tries to get the rank series for the given stock symbol.
 * If the value is not in the map, it will return null.
 *
 * @param map The map.
 * @param stock_symbol The stock symbol.
 * @return The rank series for the stock if it is in the map, else null.
 */
StockRanks* get_from_ranks_hash_map(
    const SymbolToRanksHashMap* map,
    const char* stock_symbol
) {
    size_t i = 0;
    const long key = hash_symbol(stock_symbol);
    while (1) {
        const size_t index = ((size_t)key + (i * i)) % RANK_MAP_SIZE;
        StockRanks* stock_ranks = map->symbol_to_ranks[index];
        if (stock_ranks == NULL) {
            return nullptr;
        }
        if (stock_ranks->stock_symbol == stock_symbol) {
            return stock_ranks;
        }
        i++;
    }
}

int compare_by_low(const void* a, const void* b) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    const ActiveStock* stock_a = ((ActiveStock*)a);
    const ActiveStock* stock_b = ((ActiveStock*)b);
#pragma GCC diagnostic pop
    const StockDataRow* row_a =
        &stock_a->table->rows[stock_a->current_index];
    const StockDataRow* row_b =
        &stock_b->table->rows[stock_b->current_index];
    const double a_low = row_a->low;
    const double b_low = row_b->low;
    return (a_low > b_low) - (a_low < b_low);
}

/**
 * Compares the first date with the second.
 *
 * @param date1 First date
 * @param date2 Second date
 * @return true if the first date is less than or equal to the last, else false.
 */
bool is_date_less_or_equal(
    const Date* date1,
    const Date* date2
) {
    const uint16_t year1 = date1->year;
    const uint16_t year2 = date2->year;
    if (year1 != year2) {
        return year1 < year2;
    }
    const uint16_t month1 = date1->month;
    const uint16_t month2 = date2->month;
    if (month1 != month2) {
        return month1 < month2;
    }
    return date1->day <= date2->day;
}

int compare_by_date(const void* a, const void* b) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    const StockDataRow* row_a = (*(StockDataTable**)a)->rows;
    const StockDataRow* row_b = (*(StockDataTable**)b)->rows;
#pragma GCC diagnostic pop

    const uint16_t a_year = row_a->date.year;
    const uint16_t b_year = row_b->date.year;
    if (a_year != b_year) {
        return a_year - b_year;
    }

    const uint8_t a_month = row_a->date.month;
    const uint8_t b_month = row_b->date.month;
    if (a_month != b_month) {
        return a_month - b_month;
    }

    return row_a->date.day - row_b->date.day;
}

/**
 * Compares the first date with the second.
 *
 * @param date1 First date
 * @param date2 Second date
 * @return true if the first date is less than the last, else false.
 */
bool is_date_less(
    const Date* date1,
    const Date* date2
) {
    const uint16_t year1 = date1->year;
    const uint16_t year2 = date2->year;
    if (year1 != year2) {
        return year1 < year2;
    }
    const uint8_t month1 = date1->month;
    const uint8_t month2 = date2->month;
    if (month1 != month2) {
        return month1 < month2;
    };
    return date1->day < date2->day;
}

/**
 * Fetches the min and max date of all stock data table rows.
 *
 * @param stock_data_tables The stock data tables.
 * @param table_count The number of stock data tables.
 * @param min_date Will contain the min date of all stock table rows on return.
 * @param max_date Will contain the max date of all stock table rows on return.
 */
void get_min_and_max_dates(
    StockDataTable** stock_data_tables,
    const size_t table_count,
    Date* min_date,
    Date* max_date
) {
    assert(stock_data_tables != nullptr);
    assert(table_count > 0);

    // init with first
    const StockDataTable* first_table = stock_data_tables[0];
    assert(first_table != nullptr);
    assert(first_table->row_count > 0);

    const StockDataRow* first_row_in_first_table = &first_table->rows[0];
    min_date->year = first_row_in_first_table->date.year;
    min_date->month = first_row_in_first_table->date.month;
    min_date->day = first_row_in_first_table->date.day;
    max_date->year = first_row_in_first_table->date.year;
    max_date->month = first_row_in_first_table->date.month;
    max_date->day = first_row_in_first_table->date.day;

    // Find the min and max dates
    for (size_t i = 0; i < table_count; i++) {
        const StockDataTable* table = stock_data_tables[i];
        assert(table != nullptr);
        assert(table->row_count > 0);

        // Check the first row for the min date
        const StockDataRow* first_row = &table->rows[0];
        const Date* first_date = &first_row->date;

        if (is_date_less(first_date, min_date)) {
            min_date->year = first_date->year;
            min_date->month = first_date->month;
            min_date->day = first_date->day;
        }

        // Check the last row for the max date
        const StockDataRow* last_row = &table->rows[table->row_count - 1];
        const Date* last_date = &last_row->date;
        if (is_date_less(max_date, last_date)) {
            max_date->year = last_date->year;
            max_date->month = last_date->month;
            max_date->day = last_date->day;
        }
    }
}

/**
 * Ranks all stocks in valid_stock_tables by their low price for every day.
 *
 * @param all_stock_ranks The map to store the ranking results in.
 * @param valid_stock_tables The list of valid stock tables
 *                           that have more than day_per_diff rows.
 * @param valid_stock_count The number of stock tables in valid_stock_tables.
 */
void rank_valid_stocks_by_low(
    const SymbolToRanksHashMap* all_stock_ranks,
    StockDataTable** valid_stock_tables,
    const size_t valid_stock_count
) {
    Date min_date;
    Date max_date;
    get_min_and_max_dates(
        valid_stock_tables,
        valid_stock_count,
        &min_date,
        &max_date
    );

    // Sort tables by start date
    qsort(
        valid_stock_tables,
        valid_stock_count,
        sizeof(StockDataTable*),
        compare_by_date
    );

    // active_stocks needs to be freed
    // -------------------------------------------------------------------------
    ActiveStock* active_stocks =
        malloc(sizeof(ActiveStock) * valid_stock_count);
    if (active_stocks == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    size_t active_count = 0;

    // Rank stocks for every day between min date and max date
    size_t inactive_index = 0;
    Date current_date = min_date;
    while (is_date_less_or_equal(&current_date, &max_date)) {
        const uint16_t current_year = current_date.year;
        const uint8_t current_month = current_date.month;
        const uint8_t current_day = current_date.day;

        // Activate stocks starting today
        while (inactive_index < valid_stock_count) {
            const StockDataRow* row =
                &valid_stock_tables[inactive_index]->rows[0];
            const Date start_date_of_inactive = row->date;
            if (start_date_of_inactive.year == current_year &&
                start_date_of_inactive.month == current_month &&
                start_date_of_inactive.day == current_day
            ) {
                ActiveStock* active_stock = &active_stocks[active_count++];
                active_stock->table = valid_stock_tables[inactive_index++];
                active_stock->current_index = 0;
            } else {
                break;
            }
        }

        // Use previous day if data is missing for today
        for (size_t j = 0; j < active_count; ++j) {
            ActiveStock* active_stock = &active_stocks[j];
            assert(active_stock != NULL);
            const size_t current_index = active_stock->current_index;
            const StockDataTable* active_stock_table = active_stock->table;
            if (current_index < active_stock->table->row_count) {
                assert(current_index < active_stock_table->row_count);
                const StockDataRow* row =
                    &active_stock_table->rows[current_index];
                const Date* row_date = &row->date;
                if (is_date_less(&current_date, row_date)) {
                    assert(active_stock->current_index > 0);
                    active_stock->current_index--;
                }
            } else {
                assert(active_stock->current_index > 0);
                active_stock->current_index--;
            }
        }

        // Sort active stocks by their low price for today
        qsort(
            active_stocks,
            active_count,
            sizeof(ActiveStock),
            compare_by_low
        );

        // Store ranks for today
        for (
            uint32_t active_index = 0;
            active_index < active_count;
            active_index++
        ) {
            ActiveStock* active_stock = &active_stocks[active_index];
            const StockDataTable* table = active_stock->table;
            StockRanks* stock_rank = get_from_ranks_hash_map(
                all_stock_ranks,
                table->stock_symbol
            );
            assert(stock_rank != NULL);
            const size_t current_rank_index = stock_rank->current_index;
            const StockDataRow* row = &table->rows[active_stock->current_index];

            assert(current_rank_index < stock_rank->data_size);
            if (row->date.year != 0) {
                stock_rank->rank_per_day[current_rank_index] = active_index;
                stock_rank->low_per_day[current_rank_index] = row->low;
                stock_rank->high_per_day[current_rank_index] = row->high;
                stock_rank->dates[current_rank_index] = &row->date;
                stock_rank->current_index++;
                active_stock->current_index++;
            }
        }
        next_day(&current_date);
    }

    // Allocate space for the rank diffs and went up series
    for (size_t j = 0; j < RANK_MAP_SIZE; j++) {
        StockRanks* stock_ranks = all_stock_ranks->symbol_to_ranks[j];
        if (stock_ranks) {
            const size_t rank_data_size = stock_ranks->current_index;

            // These are freed by the caller
            stock_ranks->rank_diffs =
                calloc(rank_data_size, sizeof(int64_t));
            if (stock_ranks->rank_diffs == NULL) {
                perror("calloc failed");
                exit(EXIT_FAILURE);
            }
            stock_ranks->went_up =
                calloc(rank_data_size, sizeof(bool));
            if (stock_ranks->went_up == NULL) {
                perror("calloc failed");
                exit(EXIT_FAILURE);
            }
            stock_ranks->data_size = rank_data_size;
        }
    }

    // active_stocks freed
    // -------------------------------------------------------------------------
    free(active_stocks);
}

/**
 * Ranks stocks by their low price for every day in the data.
 * Auxiliary data is added to help with backtesting.
 *
 * @param stock_data_tables The stock data tables.
 * @param all_stock_ranks The hash map to put the ranks for each stock into.
 * @param days_per_diff Number of days to look back when computing rank diffs.
 * @param buy_sell_lag The number of days after a diff to query the price
 *                     for a rank's went_up.
 */
void rank_stocks_by_low(
    const StockDataTables* stock_data_tables,
    const SymbolToRanksHashMap* all_stock_ranks,
    const size_t days_per_diff,
    const size_t buy_sell_lag
) {
    // Sort stock data by start date in the inactive array
    // Stocks are pulled out of inactive when the date they start at comes up
    const size_t number_of_stocks = stock_data_tables->table_count;

    // inactive_stock_tables needs to be freed
    // -------------------------------------------------------------------------
    StockDataTable** inactive_stock_tables =
        malloc(sizeof(StockDataTable*) * number_of_stocks);
    if (inactive_stock_tables == NULL) {
        perror("Failed to allocate memory for inactive_stock_tables");
        exit(EXIT_FAILURE);
    }

    size_t valid_count = 0;
    for (size_t i = 0; i < number_of_stocks; i++) {
        StockDataTable* table = &stock_data_tables->tables[i];
        if (table->row_count > days_per_diff) {
            inactive_stock_tables[valid_count++] = table;
        }
    }
    rank_valid_stocks_by_low(
        all_stock_ranks,
        inactive_stock_tables,
        valid_count
    );

    create_rank_diffs(
        all_stock_ranks,
        days_per_diff,
        buy_sell_lag
    );


    // inactive_stock_tables needs to be freed
    // -------------------------------------------------------------------------
    free(inactive_stock_tables);
}

/**
 * Hashes the given symbol.
 *
 * @param symbol The symbol to hash.
 * @return The hash of the given symbol.
 */
uint16_t hash_symbol(const char* symbol) {
    // Hash was over 2 times faster than gpref in production testing
    uint16_t hash = 0;
    for (int i = 0; symbol[i]; i++) {
        hash = (hash << 5) ^ (uint16_t)(symbol[i] - 'a');
    }
    return hash % RANK_MAP_SIZE;
}

/**
 * Adds the given stock ranks to the given hash map.
 *
 * @param symbol_to_ranks_map The map to add the stack ranks to.
 * @param stock_ranks The stock ranks to add to the map.
 */
void add_to_rank_hash_map(
    SymbolToRanksHashMap* symbol_to_ranks_map,
    StockRanks* stock_ranks
) {
    const char* string_key = stock_ranks->stock_symbol;
    const uint16_t key = hash_symbol(string_key);

    size_t i = 0;
    size_t index = (key + (i * i)) % RANK_MAP_SIZE;
    while (symbol_to_ranks_map->symbol_to_ranks[index] != NULL) {
        // slot is occupied
        i++;
        index = (key + (i * i)) % RANK_MAP_SIZE;
    }
    assert(symbol_to_ranks_map->symbol_to_ranks[index] == NULL);
    symbol_to_ranks_map->symbol_to_ranks[index] = stock_ranks;
    symbol_to_ranks_map->count++;
    assert(symbol_to_ranks_map->count < RANK_MAP_SIZE / 2);
}

/**
 * Allocates space for the rank data in the given symbol_to_ranks_hash_map.
 * This must be freed by the caller using free_symbol_to_ranks_hash_map.
 *
 * @param stock_data_tables The stock data to be used
 *                          to determine how much space is needed
 *                          in the symbol_to_ranks_hash_map.
 * @param symbol_to_ranks_hash_map The SymbolToRanksHashMap to initialize.
 */
void initialize_symbol_to_ranks_hash_map(
    const StockDataTables* stock_data_tables,
    SymbolToRanksHashMap* symbol_to_ranks_hash_map
) {
    symbol_to_ranks_hash_map->count = 0;
    memset(symbol_to_ranks_hash_map->symbol_to_ranks, 0, sizeof(symbol_to_ranks_hash_map->symbol_to_ranks));

    // * 2 to fill in missing days; i.e. weekends or unreported days
    constexpr size_t number_of_rows = LARGEST_STOCK_DATASET_SIZE * 2;
    const size_t number_of_stocks = stock_data_tables->table_count;
    for (size_t i = 0; i < number_of_stocks; ++i) {
        const StockDataTable* table = &stock_data_tables->tables[i];

        StockRanks* stock_ranks = malloc(sizeof(StockRanks));
        if (stock_ranks == NULL) {
            perror("Failed to allocate memory for stock_ranks");
            exit(EXIT_FAILURE);
        }
        stock_ranks->stock_symbol = table->stock_symbol;
        stock_ranks->rank_per_day = calloc(
            number_of_rows,
            sizeof(size_t)
        );
        if (stock_ranks->rank_per_day == NULL) {
            perror("Failed to allocate memory for stock_ranks->rank_per_day");
            exit(EXIT_FAILURE);
        }
        stock_ranks->low_per_day = calloc(
            number_of_rows,
            sizeof(double)
        );
        if (stock_ranks->low_per_day == NULL) {
            perror("Failed to allocate memory for stock_ranks->low_per_day");
            exit(EXIT_FAILURE);
        }
        stock_ranks->high_per_day = calloc(
            number_of_rows,
            sizeof(double)
        );
        if (stock_ranks->high_per_day == NULL) {
            perror("Failed to allocate memory for stock_ranks->high_per_day");
            exit(EXIT_FAILURE);
        }
        stock_ranks->dates = calloc(
            number_of_rows,
            sizeof(Date*)
        );
        if (stock_ranks->dates == NULL) {
            perror("Failed to allocate memory for stock_ranks->dates");
            exit(EXIT_FAILURE);
        }
        stock_ranks->current_index = 0;
        stock_ranks->data_size = number_of_rows;
        // rank_diffs and went_up are initialized after generating ranks
        stock_ranks->rank_diffs = nullptr;
        stock_ranks->went_up = nullptr;
        add_to_rank_hash_map(
            symbol_to_ranks_hash_map,
            stock_ranks
        );
    }
}
