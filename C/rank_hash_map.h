#ifndef SPS2025_RANK_HASH_MAP_H
#define SPS2025_RANK_HASH_MAP_H

#include "config.h"
#include "file_util.h"

typedef struct {
    char* stock_symbol;
    int64_t* rank_per_day;
    double* low_per_day;
    double* high_per_day;
    const Date** dates;
    int64_t* rank_diffs;
    bool* went_up;
    size_t current_index;
    size_t capacity;
} StockRanks;

typedef struct {
    StockRanks* symbol_to_ranks[RANK_MAP_SIZE];
    size_t count;
} SymbolToRanksHashMap;

void initialize_symbol_to_ranks_hash_map(
    const StockDataTables* stock_data_tables,
    SymbolToRanksHashMap* symbol_to_ranks_hash_map
);

void add_to_rank_hash_map(
    SymbolToRanksHashMap* symbol_to_ranks_map,
    StockRanks* stock_ranks
);

uint16_t hash_symbol(const char* symbol);

StockRanks* get_from_ranks_hash_map(
    const SymbolToRanksHashMap* map,
    const char* stock_symbol
);

void free_symbol_to_ranks_hash_map(SymbolToRanksHashMap* symbol_to_ranks_hash_map);

#endif //SPS2025_RANK_HASH_MAP_H
