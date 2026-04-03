#include "ranks_test.h"

#include <assert.h>

#include "../C/ranks.h"

#include <stdio.h>

#include "../C/config.h"

#include <stdlib.h>
#include <string.h>

void
rank_stocks_by_low_with_different_sized_inputs_fills_rank_diffs_and_went_up_correctly() {
    SymbolToRanksHashMap* map = malloc(sizeof(SymbolToRanksHashMap));
    if (map == NULL) {
        perror("Failed to allocate map");
        exit(EXIT_FAILURE);
    }
    map->count = 0;

    // Stock a
    StockDataRow rows_for_a[] = {
        {{2023, 1, 2}, 9, 16, 14, 9, 100},
        {{2023, 1, 4}, 13, 18, 13, 13, 100},
        {{2023, 1, 5}, 14, 22, 21, 14, 100},
        {{2023, 1, 6}, 9, 19, 16, 9, 100},
        {{2023, 1, 7}, 13, 18, 13, 13, 100},
        {{2023, 1, 8}, 14, 22, 21, 14, 100},
        {{2023, 1, 9}, 9, 23, 22, 9, 100}
    };
    const StockDataTable table_for_a = {"a", rows_for_a, 7, 7};

    // Stock c
    StockDataRow rows_for_b[] = {
        {{2023, 1, 1}, 10, 15, 14, 10, 100},
        {{2023, 1, 2}, 12, 16, 13, 12, 100},
        {{2023, 1, 3}, 11, 14, 12, 11, 100},
        {{2023, 1, 4}, 9, 21, 20, 9, 100},
        {{2023, 1, 5}, 10, 15, 14, 10, 100},
        {{2023, 1, 6}, 12, 18, 17, 12, 100},
        {{2023, 1, 7}, 11, 14, 12, 11, 100},
        {{2023, 1, 8}, 9, 21, 20, 9, 100}
    };

    const StockDataTable table_for_b = {"b", rows_for_b, 8, 8};

    const StockDataTables stock_tables = {
        .tables = (StockDataTable[]){table_for_a, table_for_b},
        .table_count = 2
    };
    if (!initialize_symbol_to_ranks_hash_map(&stock_tables, map)) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (!rank_stocks_by_low(&stock_tables, map, 2, 3)) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    const StockRanks* ranks_for_a =
        get_from_ranks_hash_map(map, table_for_a.stock_symbol);
    const StockRanks* ranks_for_b =
        get_from_ranks_hash_map(map, table_for_b.stock_symbol);

    // first two rows did not have previous data for the diff
    for (size_t i = 0; i < 2; i++) {
        if (ranks_for_a->rank_diffs[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 0; i < 2; i++) {
        if (ranks_for_b->rank_diffs[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }
    // last three rows did not have enough the price lookahead
    for (size_t i = 5; i < 8; i++) {
        if (ranks_for_a->rank_diffs[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 6; i < 9; i++) {
        if (ranks_for_b->rank_diffs[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }

    // ranks are    1, 1, 0, 1, 0, 1, 1, 1
    //           0, 0, 0, 1, 0, 1, 0, 0, 0
    // diffs are    0, 0, -1, 0, 0, 0, 1, 0
    //           0, 0, 0, 1, 0, 0, 0, -1, 0
    if (ranks_for_a->rank_diffs[2] != -1) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->rank_diffs[3] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->rank_diffs[4] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    if (ranks_for_b->rank_diffs[2] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->rank_diffs[3] != 1) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->rank_diffs[4] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->rank_diffs[5] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    // first three rows did not have previous data for the diff
    for (size_t i = 0; i < 2; i++) {
        if (ranks_for_a->went_up[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 0; i < 2; i++) {
        if (ranks_for_b->went_up[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }
    // last three rows did not have enough the price lookahead
    for (size_t i = 4; i < 8; i++) {
        if (ranks_for_a->went_up[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 5; i < 9; i++) {
        if (ranks_for_b->went_up[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }

    // went_up is    f, t
    //            t, f, t
    if (ranks_for_a->went_up[2] != false) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->went_up[3] != true) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    if (ranks_for_b->went_up[2] != false) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->went_up[3] != true) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->went_up[4] != true) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    if (ranks_for_a->size != 8) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->size != 9) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    free_symbol_to_ranks_hash_map(map);
}


void
create_rank_diffs_with_different_sized_inputs_fills_rank_diffs_and_went_up_correctly() {
    SymbolToRanksHashMap* map = malloc(sizeof(SymbolToRanksHashMap));
    if (map == NULL) {
        perror("Failed to allocate map");
        exit(EXIT_FAILURE);
    }
    map->count = 0;

    // Stock a
    StockDataRow rows_for_a[] = {
        {{2023, 1, 2}, 9, 16, 14, 9, 100},
        {{2023, 1, 4}, 13, 18, 13, 13, 100},
        {{2023, 1, 5}, 14, 22, 21, 14, 100},
        {{2023, 1, 6}, 9, 19, 16, 9, 100},
        {{2023, 1, 7}, 13, 18, 13, 13, 100},
        {{2023, 1, 8}, 14, 22, 21, 14, 100},
        {{2023, 1, 9}, 9, 23, 22, 9, 100}
    };
    const StockDataTable table_for_a = {"a", rows_for_a, 7, 7};

    // Stock b
    StockDataRow rows_for_b[] = {
        {{2023, 1, 1}, 10, 15, 14, 10, 100},
        {{2023, 1, 2}, 12, 16, 13, 12, 100},
        {{2023, 1, 3}, 11, 14, 12, 11, 100},
        {{2023, 1, 4}, 9, 21, 20, 9, 100},
        {{2023, 1, 5}, 10, 15, 14, 10, 100},
        {{2023, 1, 6}, 12, 18, 17, 12, 100},
        {{2023, 1, 7}, 11, 14, 12, 11, 100},
        {{2023, 1, 8}, 9, 21, 20, 9, 100}
    };

    const StockDataTable table_for_b = {"b", rows_for_b, 8, 8};

    const StockDataTables stock_tables = {
        .tables = (StockDataTable[]){table_for_a, table_for_b},
        .table_count = 2
    };
    if (!initialize_symbol_to_ranks_hash_map(&stock_tables, map)) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    StockDataTable* valid_tables[2] = {
        &stock_tables.tables[0],
        &stock_tables.tables[1]
    };
    if (!rank_valid_stocks_by_low(map, valid_tables, 2)) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    create_rank_diffs(
        map,
        2,
        3
    );
    const StockRanks* ranks_for_a =
        get_from_ranks_hash_map(map, table_for_a.stock_symbol);
    const StockRanks* ranks_for_b =
        get_from_ranks_hash_map(map, table_for_b.stock_symbol);

    // first two rows did not have previous data for the diff
    for (size_t i = 0; i < 2; i++) {
        if (ranks_for_a->rank_diffs[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 0; i < 2; i++) {
        if (ranks_for_b->rank_diffs[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }
    // last three rows did not have enough the price lookahead
    for (size_t i = 5; i < 8; i++) {
        if (ranks_for_a->rank_diffs[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 6; i < 9; i++) {
        if (ranks_for_b->rank_diffs[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }

    // ranks are    1, 1, 0, 1, 0, 1, 1, 1
    //           0, 0, 0, 1, 0, 1, 0, 0, 0
    // diffs are    0, 0, -1, 0, 0, 0, 1, 0
    //           0, 0, 0, 1, 0, 0, 0, -1, 0
    if (ranks_for_a->rank_diffs[2] != -1) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->rank_diffs[3] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->rank_diffs[4] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    if (ranks_for_b->rank_diffs[2] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->rank_diffs[3] != 1) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->rank_diffs[4] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->rank_diffs[5] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    // first three rows did not have previous data for the diff
    for (size_t i = 0; i < 2; i++) {
        if (ranks_for_a->went_up[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 0; i < 2; i++) {
        if (ranks_for_b->went_up[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }
    // last three rows did not have enough the price lookahead
    for (size_t i = 4; i < 8; i++) {
        if (ranks_for_a->went_up[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 5; i < 9; i++) {
        if (ranks_for_b->went_up[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }

    // went_up is    f, t
    //            t, f, t
    if (ranks_for_a->went_up[2] != false) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->went_up[3] != true) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    if (ranks_for_b->went_up[2] != false) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->went_up[3] != true) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->went_up[4] != true) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    if (ranks_for_a->size != 8) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->size != 9) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    free_symbol_to_ranks_hash_map(map);
}

void rank_valid_stocks_by_low_handles_gaps() {
    SymbolToRanksHashMap* map = malloc(sizeof(SymbolToRanksHashMap));
    if (map == NULL) {
        perror("Failed to allocate map");
        exit(EXIT_FAILURE);
    }
    map->count = 0;

    // Stock a
    StockDataRow rows_for_a[] = {
        {{2023, 1, 1}, 10, 15, 14, 10, 100},
        {{2023, 1, 2}, 12, 16, 13, 12, 100},
        {{2023, 1, 3}, 11, 14, 12, 11, 100},
        {{2023, 1, 4}, 9, 21, 20, 9, 100}
    };
    const StockDataTable table_for_a = {"a", rows_for_a, 4, 4};

    // Stock b
    StockDataRow rows_for_b[] = {
        {{2023, 1, 2}, 9, 16, 14, 9, 100},
        {{2023, 1, 4}, 13, 18, 13, 13, 100},
        {{2023, 1, 5}, 14, 22, 21, 14, 100}
    };
    const StockDataTable table_for_b = {"b", rows_for_b, 3, 3};

    const StockDataTables stock_tables = {
        .tables = (StockDataTable[]){table_for_a, table_for_b},
        .table_count = 2
    };
    if (!initialize_symbol_to_ranks_hash_map(&stock_tables, map)) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    StockDataTable* valid_tables[2] = {
        &stock_tables.tables[0],
        &stock_tables.tables[1]
    };
    if (!rank_valid_stocks_by_low(map, valid_tables, 2)) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    const StockRanks* ranks_for_a =
        get_from_ranks_hash_map(map, table_for_a.stock_symbol);
    const StockRanks* ranks_for_b =
        get_from_ranks_hash_map(map, table_for_b.stock_symbol);

    // Rank
    if (ranks_for_a->rank_per_day[0] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->rank_per_day[1] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->rank_per_day[2] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->rank_per_day[3] != 1) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->rank_per_day[4] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    if (ranks_for_b->rank_per_day[0] != 1) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->rank_per_day[1] != 1) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->rank_per_day[2] != 0) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->rank_per_day[3] != 1) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    // Low
    if (ranks_for_a->low_per_day[0] != 14) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->low_per_day[1] != 13) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->low_per_day[2] != 12) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->low_per_day[3] != 20) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->low_per_day[4] != 20) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    if (ranks_for_b->low_per_day[0] != 14) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->low_per_day[1] != 14) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->low_per_day[2] != 13) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->low_per_day[3] != 21) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    // High
    if (ranks_for_a->high_per_day[0] != 15) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->high_per_day[1] != 16) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->high_per_day[2] != 14) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->high_per_day[3] != 21) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->high_per_day[4] != 21) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    if (ranks_for_b->high_per_day[0] != 16) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->high_per_day[1] != 16) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->high_per_day[2] != 18) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->high_per_day[3] != 22) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
#pragma GCC diagnostic pop

    // Dates
    if (ranks_for_a->dates[0] != &rows_for_a[0].date) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->dates[1] != &rows_for_a[1].date) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->dates[2] != &rows_for_a[2].date) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->dates[3] != &rows_for_a[3].date) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->dates[4] != &rows_for_a[3].date) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }


    if (ranks_for_b->dates[0] != &rows_for_b[0].date) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->dates[1] != &rows_for_b[0].date) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->dates[2] != &rows_for_b[1].date) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->dates[3] != &rows_for_b[2].date) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    // rank diffs
    if (ranks_for_a->rank_diffs == NULL) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->went_up == NULL) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_a->size != 5) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < 5; i++) {
        if (ranks_for_a->rank_diffs[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }

    // went up
    if (ranks_for_b->rank_diffs == NULL) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->went_up == NULL) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    if (ranks_for_b->size != 4) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < 4; i++) {
        if (ranks_for_b->rank_diffs[i] != 0) {
            free_symbol_to_ranks_hash_map(map);
            exit(EXIT_FAILURE);
        }
    }

    free_symbol_to_ranks_hash_map(map);
}


void next_day_increments_date_correctly() {
    // New month
    Date d1 = {2023, 1, 31};
    increment_date_by_one_day(&d1);
    assert(d1.year == 2023 && d1.month == 2 && d1.day == 1);

    // Leap year
    Date d2 = {2024, 2, 28};
    increment_date_by_one_day(&d2);
    assert(d2.year == 2024 && d2.month == 2 && d2.day == 29);
    increment_date_by_one_day(&d2);
    assert(d2.year == 2024 && d2.month == 3 && d2.day == 1);

    // Non leap year
    Date d3 = {1900, 2, 28};
    increment_date_by_one_day(&d3);
    assert(d3.year == 1900 && d3.month == 3 && d3.day == 1);

    // New year
    Date d4 = {1999, 12, 31};
    increment_date_by_one_day(&d4);
    assert(d4.year == 2000 && d4.month == 1 && d4.day == 1);

    // New day
    Date d5 = {2023, 6, 15};
    increment_date_by_one_day(&d5);
    assert(d5.year == 2023 && d5.month == 6 && d5.day == 16);
}

void is_leap_year_returns_correct_result() {
    assert(is_leap_year(2024) == true);
    assert(is_leap_year(2023) == false);
    assert(is_leap_year(1900) == false);
    assert(is_leap_year(2000) == true);
}

void get_from_ranks_hash_map_non_inserted_key_returns_null() {
    SymbolToRanksHashMap* map = malloc(sizeof(SymbolToRanksHashMap));
    if (map == NULL) {
        perror("Failed to allocate map");
        exit(EXIT_FAILURE);
    }
    map->count = 0;
    memset(map->symbol_to_ranks, 0, sizeof(map->symbol_to_ranks));

    auto const inserted_symbol = "abcd";
    StockRanks* sr = malloc(sizeof(StockRanks));
    if (sr == NULL) {
        free_symbol_to_ranks_hash_map(map);
        perror("Failed to allocate map");
        exit(EXIT_FAILURE);
    }
    memset(sr, 0, sizeof(StockRanks));
    sr->stock_symbol = inserted_symbol;

    if (!add_to_rank_hash_map(map, sr)) {
        free_symbol_to_ranks_hash_map(map);
        free_stock_ranks(sr);
        exit(EXIT_FAILURE);
    }

    // Find a different key with the same hash
    const uint16_t target_hash = hash_symbol(inserted_symbol);
    char not_inserted_symbol[5] = "baaa";
    while (true) {
        if (hash_symbol(not_inserted_symbol) == target_hash) {
            break;
        }
        for (int i = 3; i >= 0; i--) {
            if (++not_inserted_symbol[i] <= 'z') {
                break;
            }
            not_inserted_symbol[i] = 'a';
        }
    }

    const StockRanks* result =
        get_from_ranks_hash_map(map, not_inserted_symbol);
    if (result != NULL) {
        free_symbol_to_ranks_hash_map(map);
        free_stock_ranks(sr);
        exit(EXIT_FAILURE);
    }
    free_symbol_to_ranks_hash_map(map);
}


void compare_by_low_sorts_correctly() {
    StockDataRow rows1[] = {{.low = 10.0}};
    StockDataRow rows2[] = {{.low = 5.0}};
    StockDataRow rows3[] = {{.low = 7.0}};

    StockDataTable t1 = {.rows = rows1};
    StockDataTable t2 = {.rows = rows2};
    StockDataTable t3 = {.rows = rows3};

    const ActiveStock s1 = {.table = &t1, .current_index = 0};
    const ActiveStock s2 = {.table = &t2, .current_index = 0};
    const ActiveStock s3 = {.table = &t3, .current_index = 0};

    ActiveStock stocks[] = {s1, s2, s3};

    qsort(stocks, 3, sizeof(ActiveStock), compare_by_low);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    assert(stocks[0].table->rows[0].low == 5.0);
    assert(stocks[1].table->rows[0].low == 7.0);
    assert(stocks[2].table->rows[0].low == 10.0);
#pragma GCC diagnostic pop
}

void is_date_less_or_equal_compares_correctly() {
    assert(
        is_date_less_or_equal(
            &(Date){ 2023, 5, 1},
            &(Date){ 2023, 5, 1}
        ) == true
    );
    assert(
        is_date_less_or_equal(
            &(Date){ 2022, 5, 1},
            &(Date){ 2023, 5, 1}
        ) == true
    );
    assert(
        is_date_less_or_equal(
            &(Date){ 2023, 4, 1},
            &(Date){ 2023, 5, 1}
        ) == true
    );
    assert(
        is_date_less_or_equal(
            &(Date){ 2023, 5, 1},
            &(Date){ 2023, 5, 2}
        ) == true
    );
    assert(
        is_date_less_or_equal(
            &(Date){ 2024, 5, 1},
            &(Date){ 2023, 5, 1}
        ) == false
    );
    assert(
        is_date_less_or_equal(
            &(Date){ 2023, 6, 1},
            &(Date){ 2023, 5, 1}
        ) == false
    );
    assert(
        is_date_less_or_equal(
            &(Date){ 2023, 5, 2},
            &(Date){ 2023, 5, 1}
        ) == false
    );
}

void compare_by_date_sorts_correctly() {
    StockDataRow row1 = {.date = {2023, 5, 10}};
    StockDataRow row2 = {.date = {2022, 6, 15}};
    StockDataRow row3 = {.date = {2022, 5, 5}};
    StockDataRow row4 = {.date = {2023, 5, 11}};

    StockDataTable t1 = {.rows = &row1};
    StockDataTable t2 = {.rows = &row2};
    StockDataTable t3 = {.rows = &row3};
    StockDataTable t4 = {.rows = &row4};

    StockDataTable* tables[] = {&t1, &t2, &t3, &t4};

    qsort(tables, 4, sizeof(StockDataTable*), compare_by_date);

    assert(tables[0]->rows->date.year == 2022);
    assert(tables[0]->rows->date.month == 5);
    assert(tables[0]->rows->date.day == 5);

    assert(tables[1]->rows->date.year == 2022);
    assert(tables[1]->rows->date.month == 6);
    assert(tables[1]->rows->date.day == 15);

    assert(tables[2]->rows->date.year == 2023);
    assert(tables[2]->rows->date.month == 5);
    assert(tables[2]->rows->date.day == 10);

    assert(tables[3]->rows->date.year == 2023);
    assert(tables[3]->rows->date.month == 5);
    assert(tables[3]->rows->date.day == 11);
}

void is_date_less_compares_correctly() {
    assert(
        is_date_less(
            &(Date){ 2023, 5, 1},
            &(Date){ 2023, 5, 1}
        ) == false
    );
    assert(
        is_date_less(
            &(Date){ 2022, 5, 1},
            &(Date){ 2023, 5, 1}
        ) == true
    );
    assert(
        is_date_less(
            &(Date){ 2023, 4, 1},
            &(Date){ 2023, 5, 1}
        ) == true
    );
    assert(
        is_date_less(
            &(Date){ 2023, 5, 1},
            &(Date){ 2023, 5, 2}
        ) == true
    );
    assert(
        is_date_less(
            &(Date){ 2024, 5, 1},
            &(Date){ 2023, 5, 1}
        ) == false
    );
    assert(
        is_date_less(
            &(Date){ 2023, 6, 1},
            &(Date){ 2023, 5, 1}
        ) == false
    );
    assert(
        is_date_less(
            &(Date){ 2023, 5, 2},
            &(Date){ 2023, 5, 1}
        ) == false
    );
}

void get_min_and_max_dates_finds_min_and_max() {
    StockDataRow rows1[] = {
        {.date = {2023, 5, 1}},
        {.date = {2023, 5, 10}},
    };
    StockDataTable t1 = {.rows = rows1, .row_count = 2};

    StockDataRow rows2[] = {
        {.date = {2022, 4, 21}},
        {.date = {2022, 5, 5}},
    };
    StockDataTable t2 = {.rows = rows2, .row_count = 2};

    StockDataRow rows3[] = {
        {.date = {2024, 1, 1}},
        {.date = {2024, 2, 1}},
    };
    StockDataTable t3 = {.rows = rows3, .row_count = 2};

    StockDataTable* tables[] = {&t1, &t2, &t3};
    Date min, max;
    get_min_and_max_dates(tables, 3, &min, &max);

    assert(min.year == 2022);
    assert(min.month == 4);
    assert(min.day == 21);

    assert(max.year == 2024);
    assert(max.month == 2);
    assert(max.day == 1);

    // This covers a bug found in another test
    StockDataRow rows_for_a[] = {
        {{2023, 1, 1}, 10, 15, 14, 10, 100},
        {{2023, 1, 2}, 12, 16, 13, 12, 100},
        {{2023, 1, 3}, 11, 14, 12, 11, 100},
        {{2023, 1, 4}, 9, 21, 20, 9, 100}
    };
    StockDataTable table_for_a = {.rows = rows_for_a, .row_count = 4};

    StockDataRow rows_for_b[] = {
        {{2023, 1, 2}, 9, 16, 14, 9, 100},
        {{2023, 1, 4}, 13, 18, 13, 13, 100},
        {{2023, 1, 5}, 14, 22, 21, 14, 100}
    };
    StockDataTable table_for_b = {.rows = rows_for_b, .row_count = 3};

    StockDataTable* tables2[] = {&table_for_a, &table_for_b};
    get_min_and_max_dates(tables2, 2, &min, &max);

    assert(min.year == 2023);
    assert(min.month == 1);
    assert(min.day == 1);

    assert(max.year == 2023);
    assert(max.month == 1);
    assert(max.day == 5);
}


void add_to_rank_hash_map_duplicates_insert() {
    SymbolToRanksHashMap* map = malloc(sizeof(SymbolToRanksHashMap));
    if (map == NULL) {
        perror("Failed to allocate map");
        exit(EXIT_FAILURE);
    }
    map->count = 0;
    memset(map->symbol_to_ranks, 0, sizeof(map->symbol_to_ranks));

    StockRanks* sr1 = malloc(sizeof(StockRanks));
    if (sr1 == NULL) {
        free_symbol_to_ranks_hash_map(map);
        perror("Failed to allocate stock ranks");
        exit(EXIT_FAILURE);
    }
    memset(sr1, 0, sizeof(StockRanks));
    sr1->stock_symbol = "aapl";
    StockRanks* sr2 = malloc(sizeof(StockRanks));
    if (sr2 == NULL) {
        free_symbol_to_ranks_hash_map(map);
        free_stock_ranks(sr1);
        perror("Failed to allocate stock ranks");
        exit(EXIT_FAILURE);
    }
    memset(sr2, 0, sizeof(StockRanks));
    sr2->stock_symbol = "msft";
    StockRanks* sr3 = malloc(sizeof(StockRanks));
    if (sr3 == NULL) {
        free_symbol_to_ranks_hash_map(map);
        free_stock_ranks(sr1);
        free_stock_ranks(sr2);
        perror("Failed to allocate stock ranks");
        exit(EXIT_FAILURE);
    }
    memset(sr3, 0, sizeof(StockRanks));
    sr3->stock_symbol = "aapl";

    if (!add_to_rank_hash_map(map, sr1) ||
        !add_to_rank_hash_map(map, sr2) ||
        !add_to_rank_hash_map(map, sr3)
    ) {
        free_symbol_to_ranks_hash_map(map);
        exit(EXIT_FAILURE);
    }

    const StockRanks* r1 = get_from_ranks_hash_map(map, "aapl");
    const StockRanks* r2 = get_from_ranks_hash_map(map, "msft");

    if (map->count != 3 ||
        r1 == NULL ||
        r1 != sr1 ||
        r2 == NULL ||
        r2 != sr2
    ) {
        free_symbol_to_ranks_hash_map(map);
        free_stock_ranks(sr1);
        free_stock_ranks(sr2);
        free_stock_ranks(sr3);
        exit(EXIT_FAILURE);
    }
    free_symbol_to_ranks_hash_map(map);
}

void initialize_symbol_to_ranks_hash_map_correctly_initializes() {
    auto const stock_symbol = "fake";

    StockDataTable* stock_table = malloc(sizeof(StockDataTable));
    assert(stock_table != NULL);
    stock_table->stock_symbol = strdup(stock_symbol);
    stock_table->rows = nullptr;
    stock_table->row_count = 1;

    StockDataTables* stock_data_tables =
        malloc(sizeof(StockDataTables));
    assert(stock_data_tables != NULL);
    stock_data_tables->tables = stock_table;
    stock_data_tables->table_count = 1;

    SymbolToRanksHashMap* symbol_to_ranks_map =
        malloc(sizeof(SymbolToRanksHashMap));
    assert(symbol_to_ranks_map != NULL);
    symbol_to_ranks_map->count = 0;

    if (!initialize_symbol_to_ranks_hash_map(
            stock_data_tables,
            symbol_to_ranks_map
        )
    ) {
        free_stock_data_tables(stock_data_tables);
        free_symbol_to_ranks_hash_map(symbol_to_ranks_map);
        exit(EXIT_FAILURE);
    }

    assert(symbol_to_ranks_map->count == 1);

    const StockRanks* stock_ranks = get_from_ranks_hash_map(
        symbol_to_ranks_map,
        stock_table->stock_symbol
    );
    assert(stock_ranks);
    assert(stock_ranks->current_index == 0);
    assert(stock_ranks->size == LARGEST_STOCK_DATASET_SIZE * 2);
    assert(stock_ranks->stock_symbol == stock_table->stock_symbol);
    assert(stock_ranks->rank_per_day != NULL);
    assert(stock_ranks->low_per_day != NULL);
    assert(stock_ranks->high_per_day != NULL);
    assert(stock_ranks->dates != NULL);
    assert(stock_ranks->rank_diffs == NULL);
    assert(stock_ranks->went_up == NULL);

    free_symbol_to_ranks_hash_map(symbol_to_ranks_map);
    free_stock_data_tables(stock_data_tables);
}

void run_ranks_tests() {
    initialize_symbol_to_ranks_hash_map_correctly_initializes();
    add_to_rank_hash_map_duplicates_insert();
    get_min_and_max_dates_finds_min_and_max();
    is_date_less_compares_correctly();
    compare_by_date_sorts_correctly();
    is_date_less_or_equal_compares_correctly();
    compare_by_low_sorts_correctly();
    get_from_ranks_hash_map_non_inserted_key_returns_null();
    is_leap_year_returns_correct_result();
    next_day_increments_date_correctly();
    rank_valid_stocks_by_low_handles_gaps();
    create_rank_diffs_with_different_sized_inputs_fills_rank_diffs_and_went_up_correctly();
    rank_stocks_by_low_with_different_sized_inputs_fills_rank_diffs_and_went_up_correctly();
}
