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
    u_int8_t* direction_data_array;
    size_t data_size;
} DirectionDataArray;

typedef struct {
    DirectionDataArray* direction_data_arrays;
    size_t data_size;
} AllDirectionDataArrays;

typedef struct {
    char* stock_symbol;
    long* profit_streak_array;
    size_t data_size;
} ProfitStreakArray;

typedef struct {
    ProfitStreakArray* profit_streaks_arrays;
    size_t data_size;
} AllProfitStreakArrays;

typedef struct {
    char* stock_symbol;
    double direction_count_array[512];
} DirectionCountArray;

typedef struct {
    DirectionCountArray* direction_counts_array;
    size_t data_size;
} AllDirectionCountArrays;

bool getDirectionData(
    StockDataTables** all_stock_data,
    AllDirectionDataArrays** all_direction_data
);

bool calculateDirectionCounts(
    AllDirectionDataArrays** all_direction_data,
    AllDirectionCountArrays** all_direction_counts
);

bool getProfitStreaks(
    AllDirectionDataArrays** all_direction_data,
    AllProfitStreakArrays** all_direction_streaks
);

void freeDirectionData(
    AllDirectionDataArrays* all_direction_data
);

void freeDirectionCounts(
    AllDirectionCountArrays* all_direction_counts
);

void free_direction_streaks(
    AllProfitStreakArrays* all_direction_streaks
);

#endif // PROBABILITY_H
