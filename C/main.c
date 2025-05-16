#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <sys/time.h>

#include "file_util.h"
#include "directions.h"
#include "config.h"
#include "tree.h"

void printOpenMPVersion() {
#ifdef _OPENMP
    printf("OpenMP version: %d\n", _OPENMP);
#else
    printf("OpenMP is not enabled.\n");
#endif
}

void performNaiveBayesOnEntireStockHistoryDirectionBits(
    const DirectionDataArray* all_direction_data,
    const DirectionCountsArray* all_direction_counts
) {
    const size_t number_of_stocks = all_direction_data->data_size;
    int boughtOnLoss = 0;
    int boughtOnProfit = 0;
    int total = 0;

#pragma omp parallel for default(none) shared(stderr, all_direction_data, number_of_stocks, all_direction_counts) reduction(+:boughtOnLoss, boughtOnProfit, total) num_threads(120) proc_bind(close) if(IS_PARALLEL)
    for (int i = 0; i < number_of_stocks; i++) {
        const DirectionDataRowArray* direction_data = &all_direction_data->
            direction_data_arrays[i];
        for (int k = 0; k < direction_data->data_size; k++) {
            const u_int8_t full_row = direction_data->direction_data[k];
            const u_int8_t row_without_profit = full_row &
                PROFIT_REMOVAL_BITMASK;
            const bool was_profit = full_row >> WAS_PROFIT_POSITION;

            const u_int8_t upRow = row_without_profit +
                PROFIT_EXTRACTION_BITMASK;
            const u_int8_t downRow = row_without_profit &
                PROFIT_REMOVAL_BITMASK;

            double up_probability_log = 0.0;
            double down_probability_log = 0.0;

            for (int j = 0; j < number_of_stocks; j++) {
                const DirectionCounts* direction_counts = &all_direction_counts
                    ->direction_counts[j];
                const double up_votes = direction_counts->direction_counts[
                    upRow];
                const double down_votes = direction_counts->direction_counts[
                    downRow];
                const double sum = up_votes + down_votes;

                if (sum > 0.0) {
                    if (up_votes > 0) {
                        const double up_ratio = log(
                            (double)up_votes / (double)sum);
                        if (up_probability_log > DBL_MAX - up_ratio) {
                            fprintf(
                                stderr,
                                "Overflow detected in up_probability_log\n");
                            exit(EXIT_FAILURE);
                        }
                        up_probability_log += up_ratio;
                    }
                    if (down_votes > 0) {
                        const double down_ratio = log(
                            (double)down_votes / (double)sum);
                        if (down_probability_log > DBL_MAX - down_ratio) {
                            fprintf(
                                stderr,
                                "Overflow detected in down_probability_log\n");
                            exit(EXIT_FAILURE);
                        }
                        down_probability_log += down_ratio;
                    }
                }
            }

            if (up_probability_log > down_probability_log) {
                if (!was_profit) {
                    ++boughtOnLoss;
                } else {
                    ++boughtOnProfit;
                }
            }
            ++total;
        }
    }

    const int sum = boughtOnLoss + boughtOnProfit;
    printf("Bought on loss: %f \n", (double)boughtOnLoss / (double)sum);
    printf("Bought on profit: %f \n", (double)boughtOnProfit / (double)sum);
}

void process(void) {
    RawStockDataArray* raw_stock_data_array;
    bool success = loadAllStockDataFromDisk(&raw_stock_data_array);
    if (!success) {
        return;
    }

    DirectionDataArray* all_direction_data;
    success = getDirectionData(&raw_stock_data_array, &all_direction_data);
    if (!success) {
        freeAllStockData(raw_stock_data_array);
        return;
    }
    DirectionStreakArray* direction_streak_array;
    success = getDirectionStreaks(
        &all_direction_data,
        &direction_streak_array
    );
    if (!success) {
        freeDirectionData(all_direction_data);
        freeAllStockData(raw_stock_data_array);
        return;
    }

    TreeNode* tree = create_node(false);
    for (int i = 0; i < direction_streak_array->data_size; i++) {
        const DirectionStreakRowArray* direction_streak_row_array =
            &direction_streak_array->direction_streaks_arrays[i];
        const long* data = direction_streak_row_array->direction_streaks;
        const u_long data_size = direction_streak_row_array->data_size;
        add_sequence(tree, data, data_size);
    }

    ulong total_count = 0;
    ulong count = count_empty_slots(tree, &total_count);
    printf("Empty slot ratio: %f\n", (double)count / (double)total_count);
    // print_tree(tree);

    // long max = 0;
    // long min = 0;
    // for (int i = 0; i < direction_streak_array->data_size; i++) {
    //     const DirectionStreakRowArray* direction_streak_row_array =
    //         &direction_streak_array->direction_streaks_arrays[i];
    //     const long* data = direction_streak_row_array->direction_streaks;
    //     const u_long data_size = direction_streak_row_array->data_size;
    //     for (u_long k = 0; k < data_size; k++) {
    //         max = max > data[k] ? max : data[k];
    //         min = min < data[k] ? min : data[k];
    //     }
    // }
    // printf("Max streak: %ld\n", max);
    // printf("Min streak: %ld\n", min);

    // FILE* p = popen("gnuplot -persistent", "w");
    // for (int i = 0; i < direction_streak_array->data_size; i++) {
    //     fprintf(p, "set terminal qt %d\n", i);
    //     fprintf(p, "set title 'Direction Streaks %d'\n", i);
    //     fprintf(p, "plot '-' with points pointtype 7 pointsize 0.5 title 'Direction Streaks'\n");
    //
    //     int x = 0;
    //     const DirectionStreakRowArray* direction_streak_row_array =
    //         &direction_streak_array->direction_streaks_arrays[i];
    //     const long* data = direction_streak_row_array->direction_streaks;
    //     const u_long data_size = direction_streak_row_array->data_size;
    //     for (u_long k = 0; k < data_size; k++) {
    //         fprintf(p, "%d %ld\n", x++, data[k]);
    //     }
    //     fprintf(p, "e\n");
    // }

    // DirectionCountsArray* all_direction_counts;
    // success = calculateDirectionCounts(&all_direction_data,
    //                                    &all_direction_counts);
    // if (!success) {
    //     freeDirectionData(all_direction_data);
    //     freeAllStockData(raw_stock_data_array);
    //     return;
    // }

    // performNaiveBayesOnEntireStockHistoryDirectionBits(
    //     all_direction_data,
    //     all_direction_counts
    // );

    // freeDirectionCounts(all_direction_counts);
    freeDirectionData(all_direction_data);
    freeAllStockData(raw_stock_data_array);
}

int main() {
    printOpenMPVersion();

    struct timeval start, stop;
    gettimeofday(&start, NULL);

    process();

    gettimeofday(&stop, NULL);
    const double time_taken =
        (double)(stop.tv_usec - start.tv_usec) / 1000000 +
        (double)(stop.tv_sec - start.tv_sec);
    printf("Function execution time: %f seconds\n", time_taken);

    return 0;
}
