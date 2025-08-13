#include "ranks.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "date_util.h"

/**
 * Compares two active stocks' current low for qsort.
 * @param a One active stock.
 * @param b The other active stock.
 * @return A negative number if a's current low is less than b's current low,
 *         a positive number if b's is greater than a's, else 0.
 */
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
    // Hack to turn doubles into the int required by qsort
    return (a_low > b_low) - (a_low < b_low);
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
    // Need to copy over pointers to the stock tables to a temporary list
    // because rank_valid_stocks_by_low mutates the list
    const size_t number_of_stocks = stock_data_tables->table_count;

    // stock_tables needs to be freed
    // -------------------------------------------------------------------------
    StockDataTable** stock_tables =
        malloc(sizeof(StockDataTable*) * number_of_stocks);
    if (stock_tables == NULL) {
        fprintf(
            stderr,
            "Failed to allocate memory for inactive_stock_tables"
        );
        exit(EXIT_FAILURE);
    }

    size_t valid_count = 0;
    for (size_t i = 0; i < number_of_stocks; i++) {
        StockDataTable* table = &stock_data_tables->tables[i];
        if (table->row_count > days_per_diff) {
            stock_tables[valid_count++] = table;
        }
    }

    rank_valid_stocks_by_low(
        all_stock_ranks,
        stock_tables,
        valid_count
    );

    create_rank_diffs(
        all_stock_ranks,
        days_per_diff,
        buy_sell_lag
    );

    // stock_tables needs to be freed
    // -------------------------------------------------------------------------
    free(stock_tables);
}

/**
 * Ranks all stocks in valid_stock_tables by their low price for every day.
 *
 * @param all_stock_ranks The map to store the ranking results in.
 * @param valid_stock_tables The list of valid stock tables
 *                           that have more than day_per_diff rows.
 *                           Note that this list of tables will be mutated.
 * @param valid_stock_count The number of stock tables in valid_stock_tables.
 */
void rank_valid_stocks_by_low(
    const SymbolToRanksHashMap* all_stock_ranks,
    StockDataTable** valid_stock_tables,
    const size_t valid_stock_count
) {
    // active_stocks needs to be freed
    // -------------------------------------------------------------------------
    ActiveStock* active_stocks =
        calloc(valid_stock_count, sizeof(ActiveStock));
    if (active_stocks == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    size_t active_count = 0;

    // Sort stock data by start date
    // Stocks are pulled out of valid and put into active
    // when the date they start at comes up
    qsort(
        valid_stock_tables,
        valid_stock_count,
        sizeof(StockDataTable*),
        compare_by_date
    );

    // Rank stocks for every day between min date and max date
    size_t inactive_index = 0;
    Date min_date;
    Date max_date;
    get_min_and_max_dates(
        valid_stock_tables,
        valid_stock_count,
        &min_date,
        &max_date
    );
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

        // move current index to the previous day if data is missing for today
        for (size_t j = 0; j < active_count; ++j) {
            ActiveStock* active_stock = &active_stocks[j];
            assert(active_stock != NULL);
            const size_t current_index = active_stock->current_index;
            const StockDataTable* active_stock_table = active_stock->table;
            if (current_index < active_stock_table->row_count) {
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

            assert(current_rank_index < stock_rank->capacity);

            // TODO year 0?
            if (row->date.year != 0) {
                stock_rank->rank_per_day[current_rank_index] = active_index;
                stock_rank->low_per_day[current_rank_index] = row->low;
                stock_rank->high_per_day[current_rank_index] = row->high;
                stock_rank->dates[current_rank_index] = &row->date;
                stock_rank->current_index++;
                active_stock->current_index++;
            }
        }
        increment_date_by_one_day(&current_date);
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
            stock_ranks->capacity = rank_data_size;
        }
    }

    // active_stocks freed
    // -------------------------------------------------------------------------
    free(active_stocks);
}

/**
 * Fills each rank's rank_diffs and went_up.
 *
 * @param symbol_to_ranks_map Each rank's rank_per_day, low_per_day, and
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
            // the diff requires an extra day
            // to ensure there are enough points for the diff
            stock_ranks->capacity < (days_per_diff + 1) + (1) + (buy_sell_lag)
        ) {
            continue;
        }
        for (size_t i = days_per_diff;
             // (the data size) - (the day after a diff) - (the price lag)
             i < (stock_ranks->capacity) - (1) - (buy_sell_lag);
             i++
        ) {
            stock_ranks->rank_diffs[i] =
                stock_ranks->rank_per_day[i] -
                stock_ranks->rank_per_day[i - days_per_diff];

            // the day after diff, if bought, assume worst case of high price
            const double high_day_after_diff = stock_ranks->high_per_day[i + 1];
            // after the number of lag days, if sold, assume worst case of low price
            const double low_sell_after_lag =
                stock_ranks->low_per_day[i + 1 + buy_sell_lag];
            // stored with the day the diff ends so indices line up later
            stock_ranks->went_up[i] = low_sell_after_lag > high_day_after_diff;
        }
    }
}
