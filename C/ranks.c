#include "ranks.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "file_util.h"

void create_all_stock_ranks(
    const AllStockDataArrays* all_stock_data,
    AllStockRanks** all_stock_ranks
) {
    const size_t number_of_stocks =
        all_stock_data->number_of_raw_stock_data_arrays;
    *all_stock_ranks = malloc(sizeof(AllStockRanks));
    (*all_stock_ranks)->stocks =
        malloc(sizeof(StockRanks) * number_of_stocks);
    (*all_stock_ranks)->count = number_of_stocks;

    for (size_t i = 0; i < number_of_stocks; ++i) {
        const RowArray* row_array = &all_stock_data->row_arrays[i];
        const size_t number_of_rows = row_array->data_size;
        StockRanks* stock_ranks = &(*all_stock_ranks)->stocks[i];
        stock_ranks->stock_symbol = row_array->stock_symbol;
        stock_ranks->rank_per_day = calloc(
            number_of_rows,
            sizeof(size_t)
        );
        stock_ranks->low_per_day = calloc(
            number_of_rows,
            sizeof(size_t)
        );
        stock_ranks->data_size = number_of_rows;
        stock_ranks->current_index = 0;
    }
}

void free_all_stock_ranks(AllStockRanks* ranks) {
    for (size_t i = 0; i < ranks->count; ++i) {
        free(ranks->stocks[i].stock_symbol);
        free(ranks->stocks[i].rank_per_day);
    }
    free(ranks->stocks);
    free(ranks);
}

typedef struct {
    RowArray* row_array;
    size_t current_index;
} ActiveStock;

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
    if (row_a->year != row_b->year) {
        return row_a->year - row_b->year;
    }
    if (row_a->month != row_b->month) {
        return row_a->month - row_b->month;
    }
    return row_a->day - row_b->day;
}

int compare_by_low(const void* a, const void* b) {
    const ActiveStock* stock_a = *((ActiveStock**)a);
    const ActiveStock* stock_b = *((ActiveStock**)b);
    const Row* row_a =
        &stock_a->row_array->rows[stock_a->current_index];
    const Row* row_b =
        &stock_b->row_array->rows[stock_b->current_index];
    return (row_a->low > row_b->low) - (row_a->low < row_b->low);
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
    const size_t number_of_stocks =
        all_stock_data->number_of_raw_stock_data_arrays;

    // Sort stock data by start date in the inactive array
    // Stocks are pulled out of inactive when the date they start comes up
    RowArray** inactive_stocks =
        malloc(sizeof(RowArray*) * number_of_stocks);
    for (size_t i = 0; i < number_of_stocks; i++) {
        inactive_stocks[i] = &all_stock_data->row_arrays[i];
    }
    qsort(
        inactive_stocks,
        number_of_stocks,
        sizeof(RowArray*),
        compare_by_date
    );

    ActiveStock* active_stocks =
        malloc(sizeof(ActiveStock) * number_of_stocks);
    ActiveStock** rankable_stocks =
        malloc(sizeof(ActiveStock*) * number_of_stocks);

    size_t active_count = 0;
    size_t inactive_index = 0;
    size_t rankable_count = 0;

    const RowArray* first = &all_stock_data->row_arrays[0];
    uint16_t min_year = first->rows[0].year;
    uint16_t max_year = first->rows[first->data_size - 1].year;
    uint8_t min_month = first->rows[0].month;
    uint8_t max_month = first->rows[first->data_size - 1].month;
    uint8_t min_day = first->rows[0].day;
    uint8_t max_day = first->rows[first->data_size - 1].day;

    for (size_t i = 0; i < number_of_stocks; i++) {
        const RowArray* arr = &all_stock_data->row_arrays[i];
        const Row* start = &arr->rows[0];
        const Row* end = &arr->rows[arr->data_size - 1];

        if (date_less(
                start->year,
                start->month,
                start->day,
                min_year,
                min_month,
                min_day
            )
        ) {
            min_year = start->year;
            min_month = start->month;
            min_day = start->day;
        }
        if (date_less(
                max_year,
                max_month,
                max_day,
                end->year,
                end->month,
                end->day
            )
        ) {
            max_year = end->year;
            max_month = end->month;
            max_day = end->day;
        }
    }

    uint16_t year = min_year;
    uint8_t month = min_month;
    uint8_t day = min_day;
    while (date_less_equal(year, month, day, max_year, max_month, max_day)) {
        // Activate stocks starting today
        while (inactive_index < number_of_stocks) {
            const Row* row = &inactive_stocks[inactive_index]->rows[0];
            if (row->year == year && row->month == month && row->day == day) {
                ActiveStock* active_stock = &active_stocks[active_count];
                active_stock->current_index = 0;
                active_stock->row_array = inactive_stocks[inactive_index++];
                active_count++;
            } else {
                break;
            }
        }

        // Only marks stock as rankable if they have data for today
        for (size_t j = 0; j < active_count; ++j) {
            ActiveStock* active_stock = &active_stocks[j];
            const size_t current_index = active_stock->current_index;
            const RowArray* active_stock_row = active_stock->row_array;
            if (current_index < active_stock->row_array->data_size) {
                const Row* row = &active_stock_row->rows[current_index];
                if (row->year == year &&
                    row->month == month &&
                    row->day == day
                ) {
                    rankable_stocks[rankable_count++] = active_stock;
                }
            }
        }

        // Assert the dates on the rows are for today
        // and sort the rankable
        for (size_t j = 0; j < rankable_count; j++) {
            const ActiveStock* active_stock = rankable_stocks[j];
            const size_t ci = active_stock->current_index;
            const Row* active_row = &active_stock->row_array->rows[ci];
            assert(
                active_row->day == day &&
                active_row->month == month &&
                active_row->year == year
            );
        }
        qsort(
            rankable_stocks,
            rankable_count,
            sizeof(ActiveStock*),
            compare_by_low
        );

        // Store ranks for today
        for (size_t rank = 0; rank < rankable_count; rank++) {
            const ActiveStock* active_stock = rankable_stocks[rank];
            const RowArray* row_array = active_stock->row_array;
            for (size_t j = 0; j < all_stock_ranks->count; j++) {
                StockRanks* stock_rank = &all_stock_ranks->stocks[j];
                if (strcmp(
                        row_array->stock_symbol,
                        stock_rank->stock_symbol
                    ) == 0
                ) {
                    const size_t current_index = stock_rank->current_index;
                    stock_rank->rank_per_day[current_index] = rank;
                    stock_rank->low_per_day[current_index] =
                        row_array->rows[active_stock->current_index].low;
                    stock_rank->current_index++;
                    break;
                }
            }
        }

        for (size_t j = 0; j < rankable_count; j++) {
            rankable_stocks[j]->current_index++;
        }
        rankable_count = 0;
        next_day(&year, &month, &day);
    }
    free(inactive_stocks);
    free(active_stocks);
    free(rankable_stocks);
}
