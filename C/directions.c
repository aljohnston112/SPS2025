#include "directions.h"

#include <float.h>
#include <stdio.h>

#include "file_util.h"

bool allocateDirectionDataArray(
    const RawStockDataArray* raw_stock_data_array,
    DirectionDataArray* all_direction_data
) {
    const size_t number_of_stocks = raw_stock_data_array->number_of_raw_stock_data_arrays;
    all_direction_data->data_size = number_of_stocks;
    all_direction_data->direction_data_arrays = malloc(number_of_stocks * sizeof(DirectionDataRowArray));
    if (all_direction_data->direction_data_arrays == NULL) {
        freeAllStockData(raw_stock_data_array);
        perror("malloc failed");
        return false;
    }

    for (int i = 0; i < number_of_stocks; i++) {
        const RowArray* raw_stock_data_row_array = &raw_stock_data_array->row_arrays[i];
        size_t direction_data_size = raw_stock_data_row_array->data_size - 2;
        if (raw_stock_data_row_array->data_size < 3) {
            direction_data_size = 0;
        }

        DirectionDataRowArray* direction_data_row_array = &all_direction_data->direction_data_arrays[i];
        direction_data_row_array->direction_data = malloc(
            direction_data_size * sizeof(u_int8_t)
        );
        direction_data_row_array->data_size = direction_data_size;
        direction_data_row_array->stock_symbol = raw_stock_data_row_array->stock_symbol;

        if (direction_data_row_array->direction_data == NULL) {
            for (int j = 0; j < i; j++) {
                free(all_direction_data->direction_data_arrays[j].direction_data);
            }
            free(all_direction_data->direction_data_arrays);
            freeAllStockData(raw_stock_data_array);
            perror("malloc failed");
            return false;
        }
    }
    return true;
}

void getDirectionDataForSingle(const RawStockDataArray* all_stock_data, DirectionDataArray* all_direction_data, int i) {
    const RowArray* stock_data = &all_stock_data->row_arrays[i];
    const Row* currentRow = &stock_data->rows[0];

    u_int8_t lastMonth = currentRow->month;
    u_int8_t lastDay = currentRow->day;
    double lastOpen = currentRow->open;
    double lastHigh = currentRow->high;
    double lastLow = currentRow->low;
    double lastClose = currentRow->close;
    double lastVolume = currentRow->volume;

    const DirectionDataRowArray* direction_data = &all_direction_data->direction_data_arrays[i];
    if (direction_data->data_size > 0) {
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

/**
 * @param all_stock_data The stock data to get the direction data of.
 * @param all_direction_data The direction data.
 */
bool getDirectionData(
    const RawStockDataArray* all_stock_data,
    DirectionDataArray* all_direction_data
) {
    const bool success = allocateDirectionDataArray(all_stock_data, all_direction_data);
    if (!success) {
        return false;
    }
#pragma omp parallel for default(none) shared(all_stock_data, all_direction_data) num_threads(180) proc_bind(close) if(IS_PARALLEL)
    for (int i = 0; i < all_stock_data->number_of_raw_stock_data_arrays; i++) {
        getDirectionDataForSingle(all_stock_data, all_direction_data, i);
    }
    return true;
}

bool allocateDirectionCountArrays(
    const DirectionDataArray* all_direction_data,
    DirectionCountsArray* all_direction_counts
) {
    const size_t number_of_stocks = all_direction_data->data_size;
    all_direction_counts->direction_counts = malloc(number_of_stocks * sizeof(DirectionCounts));
    all_direction_counts->data_size = number_of_stocks;
    if (all_direction_counts->direction_counts == NULL) {
        for (int j = 0; j < all_direction_data->data_size; j++) {
            free(all_direction_data->direction_data_arrays[j].direction_data);
        }
        free(all_direction_data->direction_data_arrays);
        perror("malloc failed");
        return false;
    }
    for (int i = 0; i < number_of_stocks; i++) {
        all_direction_counts->direction_counts[i].stock_symbol =
            all_direction_data->direction_data_arrays[i].stock_symbol;
    }
    return true;
}

bool calculateDirectionCounts(
    const DirectionDataArray* all_direction_data,
    DirectionCountsArray* all_direction_counts
) {
    const bool success = allocateDirectionCountArrays(all_direction_data, all_direction_counts);
    if (!success) {
        return false;
    }
    for (int i = 0; i < all_direction_data->data_size; i++) {
        const DirectionDataRowArray* direction_data = &all_direction_data->direction_data_arrays[i];
        DirectionCounts* direction_counts = &all_direction_counts->direction_counts[i];
        double* counts = direction_counts->direction_counts;
        for (int j = 0; j < 512; j++) {
            counts[j] = 0;
        }
        for (int j = 0; j < direction_data->data_size; j++) {
            const u_int8_t c = direction_data->direction_data[j];
            if (counts[c] > DBL_MAX - 1) {
                fprintf(stderr, "Overflow detected in direction counting\n");
                exit(EXIT_FAILURE);
            }
            ++counts[c];
        }
        direction_counts->stock_symbol = direction_data->stock_symbol;
    }
    return true;
}

void freeDirectionData(const DirectionDataArray* all_direction_data) {
    for (int j = 0; j < all_direction_data->data_size; j++) {
        free(all_direction_data->direction_data_arrays[j].direction_data);
    }
    free(all_direction_data->direction_data_arrays);
}

void freeDirectionCounts(const DirectionCountsArray* all_direction_counts) {
    free(all_direction_counts->direction_counts);
}