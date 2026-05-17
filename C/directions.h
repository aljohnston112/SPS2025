#ifndef DIRECTIONS_H
#define DIRECTIONS_H
#include <stddef.h>

typedef struct {
    char* stock_symbol;
    bool* went_up;
    size_t current_index;
    size_t capacity;
} StockPriceDirectionTable;

typedef struct {
    StockPriceDirectionTable* tables;
    size_t table_count;
    size_t capacity;
} StockPriceDirectionTables;

void load_data_and_extract_price_diffs();

#endif