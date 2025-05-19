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

constexpr int PROFIT_EXTRACTION_BITMASK = (1 << WAS_PROFIT_POSITION);
constexpr int PROFIT_REMOVAL_BITMASK = ~(1 << WAS_PROFIT_POSITION);

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
    long* direction_streaks;
    size_t data_size;
} DirectionStreakRowArray;

typedef struct {
    DirectionStreakRowArray* direction_streaks_arrays;
    size_t data_size;
} DirectionStreakArray;

typedef struct {
    char* stock_symbol;
    double direction_counts[512];
} DirectionCounts;

typedef struct {
    DirectionCounts* direction_counts;
    size_t data_size;
} DirectionCountsArray;

bool getDirectionData(
    RawStockDataArray** all_stock_data,
    DirectionDataArray** all_direction_data
);

bool calculateDirectionCounts(
    DirectionDataArray** all_direction_data,
    DirectionCountsArray** all_direction_counts
);

bool getDirectionStreaks(
    DirectionDataArray** all_direction_data,
    DirectionStreakArray** all_direction_streaks
);

void freeDirectionData(DirectionDataArray* all_direction_data);

void freeDirectionCounts(DirectionCountsArray* all_direction_counts);

void free_direction_streaks(DirectionStreakArray* all_direction_streaks);

#endif // PROBABILITY_H
