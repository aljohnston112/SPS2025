#include "ranks.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "file_util.h"

typedef struct {
    RowArray* row_array;
    size_t current_index;
} ActiveStock;

inline uint16_t hash_symbol(const char* s) {
    uint32_t hash = 0;
    for (int i = 0; s[i]; i++) {
        hash = (hash << 5) ^ (s[i] - 'a');
    }
    return hash % RANK_MAP_SIZE;
}

StockRanks* get_from_rank_hash_map(
    const RankHashMap* map,
    const char* stock_symbol
) {
    size_t i = 0;
    const long key = hash_symbol(stock_symbol);
    while (1) {
        const size_t index = (key + (i * i)) % RANK_MAP_SIZE;
        StockRanks* stock_ranks = map->stock_to_rank[index];
        if (stock_ranks == NULL) {
            return nullptr;
        }
        if (stock_ranks->stock_symbol == stock_symbol) {
            return stock_ranks;
        }
        i++;
    }
}

void add_to_rank_hash_map(
    RankHashMap* hash_map,
    StockRanks* stock_ranks
) {
    const char* string_key = stock_ranks->stock_symbol;
    const u_int16_t key = hash_symbol(string_key);
    size_t i = 0;
    // slot is occupied
    while (hash_map->stock_to_rank[(key + (i * i)) % RANK_MAP_SIZE] != NULL) {
        i++;
    }
    // found a spot to put the key, put it in the map
    const size_t index = (key + (i * i)) % RANK_MAP_SIZE;
    hash_map->stock_to_rank[index] = stock_ranks;
}


void create_all_stock_ranks(
    const AllStockDataArrays* all_stock_data,
    AllStockRanks** all_stock_ranks
) {
    const size_t number_of_stocks =
        all_stock_data->number_of_raw_stock_data_arrays;
    *all_stock_ranks = malloc(sizeof(AllStockRanks));
    (*all_stock_ranks)->symbol_to_rank_map =
        calloc(1, sizeof(RankHashMap));
    (*all_stock_ranks)->count = number_of_stocks;

    for (size_t i = 0; i < number_of_stocks; ++i) {
        const RowArray* row_array = &all_stock_data->row_arrays[i];
        constexpr size_t number_of_rows = LARGEST_STOCK_DATASET_SIZE * 2;

        StockRanks* stock_ranks = malloc(sizeof(StockRanks));
        stock_ranks->stock_symbol = row_array->stock_symbol;
        stock_ranks->rank_per_day = calloc(
            number_of_rows,
            sizeof(size_t)
        );
        stock_ranks->low_per_day = calloc(
            number_of_rows,
            sizeof(size_t)
        );
        stock_ranks->dates = calloc(
            number_of_rows,
            sizeof(Date*)
        );
        stock_ranks->data_size = number_of_rows;
        stock_ranks->current_index = 0;
        add_to_rank_hash_map(
            (*all_stock_ranks)->symbol_to_rank_map,
            stock_ranks
        );
    }
}

void free_all_stock_ranks(AllStockRanks* ranks) {
    for (size_t i = 0; i < RANK_MAP_SIZE; ++i) {
        StockRanks* sr = ranks->symbol_to_rank_map->stock_to_rank[i];
        if (sr) {
            free(sr->rank_per_day);
            free(sr->low_per_day);
            free(sr->dates);
            free(sr);
        }
    }
}

int is_leap_year(const uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

void next_day(uint16_t* year, uint8_t* month, uint8_t* day) {
    static const uint8_t days_in_month[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    uint8_t days_in_current_month = days_in_month[*month - 1];
    if (*month == 2 && is_leap_year(*year)) {
        days_in_current_month = 29;
    }

    if (*day < days_in_current_month) {
        (*day)++;
    } else {
        *day = 1;
        if (*month < 12) {
            (*month)++;
        } else {
            *month = 1;
            (*year)++;
        }
    }
}

int compare_by_date(const void* a, const void* b) {
    const Row* row_a = (*(RowArray**)a)->rows;
    const Row* row_b = (*(RowArray**)b)->rows;

    const u_int16_t a_year = row_a->date.year;
    const u_int16_t b_year = row_b->date.year;
    if (a_year != b_year) {
        return a_year - b_year;
    }

    const u_int8_t a_month = row_a->date.month;
    const u_int8_t b_month = row_b->date.month;
    if (a_month != b_month) {
        return a_month - b_month;
    }

    return row_a->date.day - row_b->date.day;
}

int compare_by_low(const void* a, const void* b) {
    const ActiveStock* stock_a = ((ActiveStock*)a);
    const ActiveStock* stock_b = ((ActiveStock*)b);
    const Row* row_a =
        &stock_a->row_array->rows[stock_a->current_index];
    const Row* row_b =
        &stock_b->row_array->rows[stock_b->current_index];
    const double a_low = row_a->low;
    const double b_low = row_b->low;
    return (a_low > b_low) - (a_low < b_low);
}

bool date_less(
    const uint16_t y1,
    const uint8_t m1,
    const uint8_t d1,
    const uint16_t y2,
    const uint8_t m2,
    const uint8_t d2
) {
    if (y1 != y2) return y1 < y2;
    if (m1 != m2) return m1 < m2;
    return d1 < d2;
}

static bool date_less_equal(
    const uint16_t y1,
    const uint8_t m1,
    const uint8_t d1,
    const uint16_t y2,
    const uint8_t m2,
    const uint8_t d2
) {
    if (y1 != y2) return y1 < y2;
    if (m1 != m2) return m1 < m2;
    return d1 <= d2;
}

void rank_by_low(
    const AllStockDataArrays* all_stock_data,
    const AllStockRanks* all_stock_ranks
) {
    size_t number_of_stocks =
        all_stock_data->number_of_raw_stock_data_arrays;

    // Sort stock data by start date in the inactive array
    // Stocks are pulled out of inactive when the date they start comes up
    RowArray** inactive_stocks =
        malloc(sizeof(RowArray*) * number_of_stocks);
    size_t valid_count = 0;
    for (size_t i = 0; i < number_of_stocks; i++) {
        if (all_stock_data->row_arrays[i].data_size > 0) {
            inactive_stocks[valid_count++] = &all_stock_data->row_arrays[i];
        }
    }
    number_of_stocks = valid_count;
    qsort(
        inactive_stocks,
        number_of_stocks,
        sizeof(RowArray*),
        compare_by_date
    );

    ActiveStock* active_stocks =
        malloc(sizeof(ActiveStock) * number_of_stocks);

    size_t active_count = 0;
    size_t inactive_index = 0;

    const RowArray* first = inactive_stocks[0];
    uint16_t min_year = first->rows[0].date.year;
    uint8_t min_month = first->rows[0].date.month;
    uint8_t min_day = first->rows[0].date.day;

    for (size_t i = 0; i < number_of_stocks; i++) {
        const RowArray* arr = inactive_stocks[i];
        const Row* start = &arr->rows[0];

        if (date_less(
                start->date.year,
                start->date.month,
                start->date.day,
                min_year,
                min_month,
                min_day
            )
        ) {
            min_year = start->date.year;
            min_month = start->date.month;
            min_day = start->date.day;
        }
    }

    uint16_t year = min_year;
    uint8_t month = min_month;
    uint8_t day = min_day;
    while (date_less_equal(year, month, day, 2024, 12, 30)) {
        // Activate stocks starting today
        while (inactive_index < number_of_stocks) {
            const Row* row = &inactive_stocks[inactive_index]->rows[0];
            if (row->date.year == year && row->date.month == month && row->date.
                day == day) {
                ActiveStock* active_stock = &active_stocks[active_count];
                active_stock->current_index = 0;
                active_stock->row_array = inactive_stocks[inactive_index++];
                active_count++;
            } else {
                break;
            }
        }

        // Use previous day if data is missing for today
        for (size_t j = 0; j < active_count; ++j) {
            ActiveStock* active_stock = &active_stocks[j];
            const size_t current_index = active_stock->current_index;
            const RowArray* active_stock_row = active_stock->row_array;
            if (current_index < active_stock->row_array->data_size) {
                const Row* row = &active_stock_row->rows[current_index];
                if (row->date.year != year ||
                    row->date.month != month ||
                    row->date.day != day
                ) {
                    assert(active_stock->current_index > 0);
                    active_stock->current_index--;
                }
            }
        }

        qsort(
            active_stocks,
            active_count,
            sizeof(ActiveStock),
            compare_by_low
        );

        // Store ranks for today
        for (size_t rank = 0; rank < active_count; rank++) {
            const ActiveStock* active_stock = &active_stocks[rank];
            const RowArray* row_array = active_stock->row_array;
            StockRanks* stock_rank = get_from_rank_hash_map(
                all_stock_ranks->symbol_to_rank_map,
                active_stock->row_array->stock_symbol
            );
            assert(stock_rank != NULL);
            const size_t current_index = stock_rank->current_index;
            assert(stock_rank->current_index < stock_rank->data_size);
            stock_rank->rank_per_day[current_index] = rank;
            stock_rank->low_per_day[current_index] =
                row_array->rows[active_stock->current_index].low;
            stock_rank->dates[current_index] =
                &row_array->rows[active_stock->current_index].date;
            stock_rank->current_index++;
        }

        for (size_t j = 0; j < active_count; j++) {
            active_stocks[j].current_index++;
        }
        next_day(&year, &month, &day);
    }

    for (size_t j = 0; j < RANK_MAP_SIZE; j++) {
        StockRanks* s = all_stock_ranks->symbol_to_rank_map->stock_to_rank[j];
        if (s) {
            s->data_size = s->current_index;
        }
    }

    free(inactive_stocks);
    free(active_stocks);
}
