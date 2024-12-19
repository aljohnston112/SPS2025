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


/**
 *
 * @param stockData The stock data whose records will be used to calculate the probability of a win.
 * @param stockDataSize The number of stocks whose data is in stockData.
 * @param directionData
 * @return The probability of a win.
 */
void getDirectionData(const RawStockDataResults* stockData, const int stockDataSize, DirectionData* directionData) {
    if (stockData->data_size < 1) {
        return;
    }

    for (int j = 0; j < stockDataSize; j++) {
        u_int8_t* results = malloc(stockData->data_size * sizeof(char));

        char lastMonth = stockData->stockData[0].month;
        char lastDay = stockData->stockData[0].day;
        double lastOpen = stockData->stockData[0].open;
        double lastHigh = stockData->stockData[0].high;
        double lastLow = stockData->stockData[0].low;
        double lastClose = stockData->stockData[0].close;
        double lastVolume = stockData->stockData[0].volume;
        for (int i = 1; i < stockData->data_size - 1; i++) {
            unsigned char record = 0;
            const char month = stockData->stockData[i].month;
            const char day = stockData->stockData[i].day;
            const double open = stockData->stockData[i].open;
            const double high = stockData->stockData[i].high;
            const double low = stockData->stockData[i].low;
            const double close = stockData->stockData[i].close;
            const double volume = stockData->stockData[i].volume;

            if (month > lastMonth) {
                record += (1 << MONTH_POSITION);
            }

            if (day > lastDay) {
                record += (1 << DAY_POSITION);
            }
            if (open > lastOpen) {
                record += (1 << OPEN_POSITION);
            }

            if (high > lastHigh) {
                record += (1 << HIGH_POSITION);
            }

            if (low > lastLow) {
                record += (1 << LOW_POSITION);
            }

            if (close > lastClose) {
                record += (1 << CLOSE_POSITION);
            }

            if (volume > lastVolume) {
                record += (1 << VOLUME_POSITION);
            }

            const double nextLow = stockData->stockData[i + 1].low;
            if (nextLow > lastHigh) {
                record += (1 << WAS_PROFIT_POSITION);
            }
            results[i] = record;

            lastMonth = month;
            lastDay = day;
            lastOpen = open;
            lastHigh = high;
            lastLow = low;
            lastClose = close;
            lastVolume = volume;
        }

        directionData[j].stockSymbol = stockData[j].symbol;
        directionData[j].directionData = results;
        directionData[j].dataSize = stockData[j].data_size - 1;
    }
}

#endif // PROBABILITY_H
