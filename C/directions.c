#include "directions.h"

#include <assert.h>
#include <float.h>
#include <stdio.h>

#include "config.h"
#include "file_util.h"

bool allocateDirectionDataArrays(
    const StockDataTables* raw_stock_data_array,
    AllDirectionDataArrays* all_direction_data
) {
    assert(raw_stock_data_array != NULL);
    assert(all_direction_data != NULL);

    const size_t number_of_stocks =
        raw_stock_data_array->table_count;
    all_direction_data->data_size = number_of_stocks;
    all_direction_data->direction_data_arrays = malloc(
        number_of_stocks * sizeof(DirectionDataArray)
    );
    if (all_direction_data->direction_data_arrays == NULL) {
        perror("malloc failed");
        exit(1);
    }

    for (int i = 0; i < number_of_stocks; i++) {
        const StockDataTable* raw_stock_data_row_array =
            &raw_stock_data_array->tables[i];
        size_t direction_data_size = raw_stock_data_row_array->row_count - BUY_SELL_LAG - 1;

        // + 2 since a lag of 0 means sell the next day
        // and at least two days are needed for one buy/sell cycle
        if (raw_stock_data_row_array->row_count < BUY_SELL_LAG + 2) {
            direction_data_size = 0;
        }

        DirectionDataArray* direction_data_row_array =
            &all_direction_data->direction_data_arrays[i];
        direction_data_row_array->direction_data_array = malloc(
            direction_data_size * sizeof(u_int8_t)
        );
        direction_data_row_array->data_size = direction_data_size;
        direction_data_row_array->stock_symbol =
            raw_stock_data_row_array->stock_symbol;
        if (direction_data_row_array->direction_data_array == NULL) {
            perror("malloc failed");
            exit(1);
        }
    }
    return true;
}

void getDirectionDataForSingle(
    const StockDataTables* all_stock_data,
    const AllDirectionDataArrays* all_direction_data,
    const int stock_index
) {
    assert(all_stock_data != NULL);
    assert(all_direction_data != NULL);
    assert(all_direction_data->data_size > 0);

    const DirectionDataArray* direction_data =
        &all_direction_data->direction_data_arrays[stock_index];
    if (direction_data->data_size == 0) {
        return;
    }

    const StockDataTable* stock_row_array = &all_stock_data->tables[stock_index];
    const StockDataRow* currentRow = &stock_row_array->rows[0];

    u_int8_t lastMonth = currentRow->date.month;
    u_int8_t lastDay = currentRow->date.day;
    double lastOpen = currentRow->open;
    double lastHigh = currentRow->high;
    double lastLow = currentRow->low;
    double lastClose = currentRow->close;
    double lastVolume = currentRow->volume;

    if (stock_row_array->row_count >= BUY_SELL_LAG) {
        for (int i = BUY_SELL_LAG + 1; i < stock_row_array->row_count; i++) {
            unsigned char record = 0;
            currentRow = &stock_row_array->rows[i];

            const u_int8_t month = currentRow->date.month;
            const u_int8_t day = currentRow->date.day;
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

            const double buy_day_high = stock_row_array->rows[i - BUY_SELL_LAG - 1].high;
            const double sell_day_low = low;
            if (sell_day_low > buy_day_high) {
                record += (1 << WAS_PROFIT_POSITION);
            }

            assert(i - BUY_SELL_LAG - 1 >= 0);
            direction_data->direction_data_array[i - BUY_SELL_LAG - 1] = record;

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
    StockDataTables** all_stock_data,
    AllDirectionDataArrays** all_direction_data
) {
    (*all_direction_data) = malloc(sizeof(AllDirectionDataArrays));
    const bool success = allocateDirectionDataArrays(
        *all_stock_data,
        *all_direction_data
    );
    if (!success) {
        return false;
    }
#if IS_PARALLEL
    #pragma omp parallel \
        for default(none) \
        shared(all_stock_data, all_direction_data) \
        num_threads(180) \
        proc_bind(close)
#endif
    for (int i = 0;
         i < (*all_direction_data)->data_size;
         i++
    ) {
        getDirectionDataForSingle(*all_stock_data, *all_direction_data, i);
    }
    return true;
}

bool allocateDirectionCountArrays(
    const AllDirectionDataArrays* all_direction_data,
    AllDirectionCountArrays* all_direction_counts
) {
    const size_t number_of_stocks = all_direction_data->data_size;
    all_direction_counts->direction_counts_array = malloc(
        number_of_stocks * sizeof(DirectionCountArray));
    all_direction_counts->data_size = number_of_stocks;
    if (all_direction_counts->direction_counts_array == NULL) {
        for (int j = 0; j < all_direction_data->data_size; j++) {
            free(all_direction_data->direction_data_arrays[j].direction_data_array);
        }
        free(all_direction_data->direction_data_arrays);
        perror("malloc failed");
        return false;
    }
    for (int i = 0; i < number_of_stocks; i++) {
        all_direction_counts->direction_counts_array[i].stock_symbol =
            all_direction_data->direction_data_arrays[i].stock_symbol;
    }
    return true;
}

bool allocateDirectionStreakArray(
    AllDirectionDataArrays* direction_data_array,
    AllProfitStreakArrays* direction_streaks_array
) {
    const size_t number_of_stocks = direction_data_array->data_size;
    direction_streaks_array->data_size = number_of_stocks;
    direction_streaks_array->profit_streaks_arrays = malloc(
        number_of_stocks * sizeof(ProfitStreakArray));
    if (direction_streaks_array->profit_streaks_arrays == NULL) {
        freeDirectionData(direction_data_array);
        perror("malloc failed");
        return false;
    }

    for (int i = 0; i < number_of_stocks; i++) {
        const DirectionDataArray* direction_data_row_array =
            &direction_data_array->direction_data_arrays[i];
        size_t direction_streak_size = direction_data_row_array->data_size - 2;
        if (direction_data_row_array->data_size < 3) {
            direction_streak_size = 0;
        }

        ProfitStreakArray* direction_streak_row_array = &
            direction_streaks_array->profit_streaks_arrays[i];
        direction_streak_row_array->profit_streak_array = malloc(
            direction_streak_size * sizeof(long)
        );
        direction_streak_row_array->data_size = direction_streak_size;
        direction_streak_row_array->stock_symbol = direction_data_row_array->
            stock_symbol;

        if (direction_streak_row_array->profit_streak_array == NULL) {
            for (int j = 0; j < i; j++) {
                free(direction_streaks_array->profit_streaks_arrays[j].
                    profit_streak_array);
            }
            free(direction_streaks_array->profit_streaks_arrays);
            freeDirectionData(direction_data_array);
            perror("malloc failed");
            return false;
        }
    }
    return true;
}


void getDirectionStreaksForSingle(
    const AllDirectionDataArrays* all_direction_data,
    const AllProfitStreakArrays* all_direction_streaks,
    const int i
) {
    const DirectionDataArray direction_data =
        all_direction_data->direction_data_arrays[i];
    ProfitStreakArray* direction_streaks =
        &all_direction_streaks->profit_streaks_arrays[i];
    bool was_profit = true;
    long current_streak = 0;
    u_long current_streak_index = 0;
    for (int j = 0; j < direction_data.data_size; j++) {
        const u_int8_t c = direction_data.direction_data_array[j];
        const bool is_profit = c & PROFIT_EXTRACTION_BITMASK;
        if (is_profit == was_profit) {
            current_streak++;
        } else {
            if (was_profit) {
                direction_streaks->profit_streak_array[current_streak_index] =
                    current_streak;
            } else {
                direction_streaks->profit_streak_array[current_streak_index] = -
                    current_streak;
            }
            current_streak_index++;
            current_streak = 1;
            was_profit = is_profit;
        }
    }
    direction_streaks->data_size = current_streak_index;
}

bool getProfitStreaks(
    AllDirectionDataArrays** all_direction_data,
    AllProfitStreakArrays** all_direction_streaks
) {
    (*all_direction_streaks) = malloc(sizeof(AllProfitStreakArrays));
    const bool success = allocateDirectionStreakArray(
        *all_direction_data, *all_direction_streaks);
    if (!success) {
        return false;
    }
#if IS_PARALLEL
    #pragma omp parallel \
        for default(none) \
        shared(all_direction_data, all_direction_streaks) \
        num_threads(180) \
        proc_bind(close) \
        if(IS_PARALLEL)
#endif
    for (int i = 0; i < (*all_direction_data)->data_size; i++) {
        getDirectionStreaksForSingle(
            *all_direction_data,
            *all_direction_streaks,
            i
        );
    }
    return true;
}

bool calculateDirectionCounts(
    AllDirectionDataArrays** all_direction_data,
    AllDirectionCountArrays** all_direction_counts
) {
    (*all_direction_counts) = malloc(sizeof(DirectionCountArray));
    const bool success = allocateDirectionCountArrays(
        *all_direction_data, *all_direction_counts);
    if (!success) {
        return false;
    }
    for (int i = 0; i < (*all_direction_data)->data_size; i++) {
        const DirectionDataArray* direction_data = &(*all_direction_data)->
            direction_data_arrays[i];
        DirectionCountArray* direction_counts = &(*all_direction_counts)->
            direction_counts_array[i];
        double* counts = direction_counts->direction_count_array;
        for (int j = 0; j < 512; j++) {
            counts[j] = 0;
        }
        for (int j = 0; j < direction_data->data_size; j++) {
            const u_int8_t c = direction_data->direction_data_array[j];
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

void freeDirectionData(AllDirectionDataArrays* all_direction_data) {
    for (int j = 0; j < all_direction_data->data_size; j++) {
        free(all_direction_data->direction_data_arrays[j].direction_data_array);
    }
    free(all_direction_data->direction_data_arrays);
    free(all_direction_data);
}

void freeDirectionCounts(AllDirectionCountArrays* all_direction_counts) {
    free(all_direction_counts->direction_counts_array);
    free(all_direction_counts);
}

void free_direction_streaks(AllProfitStreakArrays* all_direction_streaks) {
    for (int i = 0; i < all_direction_streaks->data_size; i++) {
        free(all_direction_streaks->profit_streaks_arrays[i].profit_streak_array);
    }
    free(all_direction_streaks->profit_streaks_arrays);
    free(all_direction_streaks);
}
