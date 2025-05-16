#ifndef ARRAY_TO_PYTHON_H
#define ARRAY_TO_PYTHON_H

#include "directions.h"
#include "file_util.h"

typedef struct {
    RawStockDataArray* raw_stock_data;
    DirectionDataArray* directions;
    DirectionCountsArray* direction_counts;
    size_t size;
} StockDataArray;

StockDataArray* load_stock_data();
void free_stock_data(StockDataArray* stock_data_array);

#endif //ARRAY_TO_PYTHON_H
