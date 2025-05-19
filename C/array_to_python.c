#include "array_to_python.h"

StockDataArray* load_stock_data() {

    RawStockDataArray* raw_stock_data_array;
    bool success = load_all_stock_data_from_disk(&raw_stock_data_array);
    if (!success) {
        return nullptr;
    }

    DirectionDataArray* all_direction_data;
    success = getDirectionData(&raw_stock_data_array, &all_direction_data);
    if (!success) {
        freeAllStockData(raw_stock_data_array);
        return nullptr;
    }

    DirectionCountsArray* all_direction_counts;
    success = calculateDirectionCounts(&all_direction_data, &all_direction_counts);
    if (!success) {
        freeDirectionData(all_direction_data);
        freeAllStockData(raw_stock_data_array);
        return nullptr;
    }

    StockDataArray* stock_data_array = malloc(sizeof(StockDataArray));
    stock_data_array->raw_stock_data = raw_stock_data_array;
    stock_data_array->directions = all_direction_data;
    stock_data_array->direction_counts = all_direction_counts;
    return stock_data_array;
}


void free_stock_data(StockDataArray* stock_data_array) {
    freeDirectionCounts(stock_data_array->direction_counts);
    freeDirectionData(stock_data_array->directions);
    freeAllStockData(stock_data_array->raw_stock_data);
    free(stock_data_array);
}