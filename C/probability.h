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
    char* stockSymbol;
    u_int8_t* directionData;
    size_t dataSize;
} DirectionData;

typedef struct {
    char* stockSymbol;
    u_int8_t directionCounts[512];
} DirectionCounts;

void getDirectionData(const RawStockDataResults* allStockData, int stockDataSize, DirectionData* allDirectionData);
void calculate_direction_counts(const DirectionData* all_direction_data, int results_count, DirectionCounts* all_direction_counts);
#endif // PROBABILITY_H
