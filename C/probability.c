#include "probability.h"

#include "file_util.h"

/**
 * @param all_stock_data The stock data to get the direction data of.
 * @param all_direction_data The direction data.
 */
void getDirectionData(
    const RawStockDataArray* all_stock_data,
    const DirectionDataArray* all_direction_data
) {
    for (int j = 0; j < all_stock_data->number_of_raw_stock_data_arrays; j++) {
        const RowArray* stock_data = &all_stock_data->row_arrays[j];
        const Row* currentRow = &stock_data->rows[0];

        u_int8_t lastMonth = currentRow->month;
        u_int8_t lastDay = currentRow->day;
        double lastOpen = currentRow->open;
        double lastHigh = currentRow->high;
        double lastLow = currentRow->low;
        double lastClose = currentRow->close;
        double lastVolume = currentRow->volume;

        const DirectionDataRowArray* direction_data = &all_direction_data->direction_data_arrays[j];
        if(direction_data->data_size > 0) {
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
                direction_data->direction_data[i - 1] = record;

                lastMonth = month;
                lastDay = day;
                lastOpen = open;
                lastHigh = high;
                lastLow = low;
                lastClose = close;
                lastVolume = volume;
            }
        }
    }
}

void calculate_direction_counts(
    const DirectionDataArray* all_direction_data,
    const DirectionCountsArray* all_direction_counts
) {
    for (int i = 0; i < all_direction_data->data_size; i++) {
        const DirectionDataRowArray* direction_data = &all_direction_data->direction_data_arrays[i];
        DirectionCounts* direction_counts = &all_direction_counts->direction_counts[i];
        u_int8_t* counts = direction_counts->direction_counts;
        for (int j = 0; j < 512; j++) {
            counts[j] = 0;
        }
        for (int j = 0; j < direction_data->data_size; j++) {
            ++counts[direction_data->direction_data[j]];
        }
        direction_counts->stock_symbol = direction_data->stock_symbol;
    }
}
