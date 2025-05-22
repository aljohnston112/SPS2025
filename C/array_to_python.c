#include "array_to_python.h"

AllStockDataContainer* load_stock_data() {

    AllStockDataArrays* raw_stock_data_array;
    bool success = load_stock_data_from_disk(&raw_stock_data_array);
    if (!success) {
        return nullptr;
    }

    AllDirectionDataArrays* all_direction_data;
    success = getDirectionData(&raw_stock_data_array, &all_direction_data);
    if (!success) {
        freeAllStockData(raw_stock_data_array);
        return nullptr;
    }

    AllDirectionCountArrays* all_direction_counts;
    success = calculateDirectionCounts(&all_direction_data, &all_direction_counts);
    if (!success) {
        freeDirectionData(all_direction_data);
        freeAllStockData(raw_stock_data_array);
        return nullptr;
    }

    AllStockDataContainer* stock_data_array = malloc(sizeof(AllStockDataContainer));
    stock_data_array->raw_stock_data = raw_stock_data_array;
    stock_data_array->directions = all_direction_data;
    stock_data_array->direction_counts = all_direction_counts;
    return stock_data_array;
}


void free_stock_data(AllStockDataContainer* stock_data_array) {
    freeDirectionCounts(stock_data_array->direction_counts);
    freeDirectionData(stock_data_array->directions);
    freeAllStockData(stock_data_array->raw_stock_data);
    free(stock_data_array);
}