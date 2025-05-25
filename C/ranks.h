#ifndef RANKS_H
#define RANKS_H

#include <stddef.h>
#include <stdint.h>

#include "file_util.h"

# define RANK_MAP_SIZE 22717

typedef struct {
    char* stock_symbol;
    size_t* rank_per_day;
    double* low_per_day;
    Date** dates;
    size_t current_index;
    size_t data_size;
} StockRanks;

typedef struct {
    StockRanks* stock_to_rank[RANK_MAP_SIZE];
} RankHashMap;

typedef struct {
    RankHashMap* symbol_to_rank_map;
    size_t count;
} AllStockRanks;

uint16_t hash_symbol(const char* s);

StockRanks* get_from_rank_hash_map(
    const RankHashMap* map,
    const char* stock_symbol
);

void add_to_rank_hash_map(
    RankHashMap* hash_map,
    StockRanks* stock_ranks
);

void create_all_stock_ranks(
    const AllStockDataArrays* all_stock_data,
    AllStockRanks** all_stock_ranks
);

void rank_by_low(
    const AllStockDataArrays* all_stock_data,
    const AllStockRanks* all_stock_ranks
);

#endif //RANKS_H
