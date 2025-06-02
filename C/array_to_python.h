#ifndef ARRAY_TO_PYTHON_H
#define ARRAY_TO_PYTHON_H

#include "directions.h"
#include "file_util.h"

typedef struct {
    StockDataTables* raw_stock_data;
    AllDirectionDataArrays* directions;
    AllDirectionCountArrays* direction_counts;
    size_t size;
} AllStockDataContainer;

AllStockDataContainer* load_stock_data();
void free_stock_data(AllStockDataContainer* stock_data_array);

#endif //ARRAY_TO_PYTHON_H
