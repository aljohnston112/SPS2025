#ifndef RANKS_H
#define RANKS_H
#include <stddef.h>

#include "file_util.h"

typedef struct {
    char* stock_symbol;
    size_t* rank_per_day;
    double * low_per_day;
    size_t current_index;
    size_t data_size;
} StockRanks;

typedef struct {
    StockRanks* stocks;
    size_t count;
} AllStockRanks;

void create_all_stock_ranks(
    const AllStockDataArrays* all_stock_data,
    AllStockRanks** all_stock_ranks
);

void rank_by_low(
    const AllStockDataArrays* all_stock_data,
    const AllStockRanks* all_stock_ranks
);

#endif //RANKS_H
