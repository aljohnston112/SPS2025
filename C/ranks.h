#ifndef RANKS_H
#define RANKS_H

#include "rank_hash_map.h"

typedef struct {
    StockDataTable* table;
    size_t current_index;
} ActiveStock;

int compare_by_low(const void* a, const void* b);

bool rank_valid_stocks_by_low(
    const SymbolToRanksHashMap* all_stock_ranks,
    StockDataTable** valid_stock_tables,
    size_t valid_stock_count
);

bool rank_stocks_by_low(
    const StockDataTables* stock_data_tables,
    const SymbolToRanksHashMap* all_stock_ranks,
    size_t days_per_diff,
    size_t buy_sell_lag
);

void create_rank_diffs(
    const SymbolToRanksHashMap* symbol_to_ranks_map,
    size_t days_per_diff,
    size_t buy_sell_lag
);

#endif //RANKS_H
