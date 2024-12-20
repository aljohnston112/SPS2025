#include "probability.h"

#include "file_util.h"

/**
 * @param allStockData The stock data to get the direction data of.
 * @param stockDataSize The number of stocks in stockData.
 * @param allDirectionData The direction data .
 */
void getDirectionData(
    const RawStockDataResults* allStockData,
    const int stockDataSize,
    DirectionData* allDirectionData
) {
    for (int j = 0; j < stockDataSize; j++) {
        const RowArray* stock_data = allStockData[j].rows;
        const Row* currentRow = &stock_data->rows[0];
        u_int8_t lastMonth = currentRow->month;
        u_int8_t lastDay = currentRow->day;
        double lastOpen = currentRow->open;
        double lastHigh = currentRow->high;
        double lastLow = currentRow->low;
        double lastClose = currentRow->close;
        double lastVolume = currentRow->volume;

        DirectionData* direction_data = &allDirectionData[j];
        direction_data->stockSymbol = allStockData[j].symbol;
        if (stock_data->data_size < 2) {
            direction_data->dataSize = 0;
        } else {
            for (int i = 1; i < stock_data->data_size - 1; i++) {
                unsigned char record = 0;
                currentRow = &stock_data->rows[i];

                const u_int8_t month = currentRow->month;
                const u_int8_t day = currentRow->day;
                const double open = currentRow->open;
                const double high = currentRow->high;
                const double low = currentRow->low;
                const double close = currentRow->close;
                const double volume = currentRow->volume;

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

                const double nextLow = stock_data->rows[i + 1].low;
                if (nextLow > lastHigh) {
                    record += (1 << WAS_PROFIT_POSITION);
                }
                direction_data->directionData[i - 1] = record;

                lastMonth = month;
                lastDay = day;
                lastOpen = open;
                lastHigh = high;
                lastLow = low;
                lastClose = close;
                lastVolume = volume;
            }
            direction_data->dataSize = stock_data->data_size - 2;
        }
    }
}

void calculate_direction_counts(
    const DirectionData* all_direction_data,
    const int results_count,
    DirectionCounts* all_direction_counts
) {
    for (int i = 0; i < results_count; i++) {
        const DirectionData* direction_data = &all_direction_data[i];
        DirectionCounts* direction_counts = &all_direction_counts[i];
        u_int8_t* counts = direction_counts->directionCounts;
        for (int j = 0; j < 512; j++) {
            counts[j] = 0;
        }
        for (int j = 0; j < direction_data->dataSize; j++) {
            ++counts[direction_data->directionData[j]];
        }
        direction_counts->stockSymbol = direction_data->stockSymbol;
    }
}
