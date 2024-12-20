#ifndef PROBABILITY_H
#define PROBABILITY_H

#include <stdlib.h>

#include "file_util.h"

enum {
    MONTH_POSITION = 0,
    DAY_POSITION = 1,
    OPEN_POSITION = 2,
    HIGH_POSITION = 3,
    LOW_POSITION = 4,
    CLOSE_POSITION = 5,
    VOLUME_POSITION = 6,
    WAS_PROFIT_POSITION = 7
};

typedef struct {
    char* stock_symbol;
    u_int8_t* direction_data;
    size_t data_size;
} DirectionDataRowArray;

typedef struct {
    DirectionDataRowArray* direction_data_arrays;
    size_t data_size;
} DirectionDataArray;

typedef struct {
    char* stock_symbol;
    u_int8_t direction_counts[512];
} DirectionCounts;

typedef struct {
    DirectionCounts* direction_counts;
    size_t direction_size;
} DirectionCountsArray;

void getDirectionData(
    const RawStockDataArray* all_stock_data,
    const DirectionDataArray* all_direction_data
);

void calculate_direction_counts(
    const DirectionDataArray* all_direction_data,
    const DirectionCountsArray* all_direction_counts
);
#endif // PROBABILITY_H
