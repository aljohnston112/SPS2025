#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <sys/time.h>

#include "file_util.h"
#include "directions.h"
#include "config.h"
#include "tree.h"

#if IS_PARALLEL
void printOpenMPVersion() {
#ifdef _OPENMP
    printf("OpenMP version: %d\n", _OPENMP);
#else
        printf("OpenMP is not enabled.\n");
#endif
}
#endif


void performNaiveBayesOnEntireStockHistoryDirectionBits(
    const DirectionDataArray* all_direction_data,
    const DirectionCountsArray* all_direction_counts
) {
    const size_t number_of_stocks = all_direction_data->data_size;
    int boughtOnLoss = 0;
    int boughtOnProfit = 0;
    int total = 0;

#if IS_PARALLEL
#pragma omp parallel for default(none) \
    shared(stderr, all_direction_data, number_of_stocks, all_direction_counts) \
    reduction(+:boughtOnLoss, boughtOnProfit, total) \
    num_threads(120) \
    proc_bind(close) \
    if(IS_PARALLEL)
#endif
    for (int i = 0; i < number_of_stocks; i++) {
        const DirectionDataRowArray* direction_data =
            &all_direction_data->direction_data_arrays[i];
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

void run_simulation(
    const HashMap* tree,
    const RawStockDataArray* future_stock_data,
    const DirectionStreakArray* future_direction_streaks
) {
    // the streak at index 1 denotes the first streak
    // that starts at data index 3 and ends at data index (3 + streak size)


    double profit = 0;

    for (size_t i = 0; i < future_direction_streaks->data_size; i++) {
        const DirectionStreakRowArray* all_stock_streak_arrays =
            future_direction_streaks->direction_streaks_arrays;
        if (all_stock_streak_arrays->data_size > 3) {
            const RawStockDataArray stock_data = future_stock_data[i];

            const DirectionStreakRowArray streak_row_array =
                all_stock_streak_arrays[i];

            const long* streak_array = streak_row_array.direction_streaks;
            u_int64_t current_day = 6;
            for (size_t j = 0; j < 3; j++) {
                const long streak = streak_array[j];
                const size_t add = streak > 0 ? streak : -streak;
                current_day += add;
            }

            const long first_streak = streak_array[3];
            assert(first_streak < 0);

            // Can't know when streak became negative
            // until one day after a negative
            const size_t add = -first_streak;
            size_t current_streak_end_data_index = current_day + add;
            for (size_t current_streak_index = 4;
                 current_streak_index < streak_row_array.data_size;
                 current_streak_index++
            ) {
                const size_t prediction = floor(
                    get_prediction_from_hash_map(
                        tree,
                        streak_array,
                        current_streak_index - 1
                    )
                );

                const long current_streak = streak_array[current_streak_index];
                if (prediction != -1) {
                    const size_t predicted_day = prediction + current_day;
                    if (predicted_day <= current_streak_end_data_index) {
                        if (current_streak < 0) {

                        } else {

                        }
                    } else {
                        if (current_streak < 0) {

                        } else {

                        }
                    }
                }
                printf("A");
            }
        }
    }
}

void process(void) {
    RawStockDataArray* past_stock_data_array;
    bool success = load_stock_data_to_year_from_disk(
        &past_stock_data_array, 2023);
    if (!success) {
        exit(1);
    }

    DirectionDataArray* past_direction_data;
    success = getDirectionData(&past_stock_data_array, &past_direction_data);
    if (!success) {
        exit(1);
    }
    DirectionStreakArray* past_direction_streak_array;
    success = getDirectionStreaks(
        &past_direction_data,
        &past_direction_streak_array
    );
    if (!success) {
        exit(1);
    }

    HashMap* tree = create_hash_map(0);
    for (int i = 0; i < past_direction_streak_array->data_size; i++) {
        const DirectionStreakRowArray* direction_streak_row_array =
            &past_direction_streak_array->direction_streaks_arrays[i];
        const size_t data_size = direction_streak_row_array->data_size;

        const long* data = direction_streak_row_array->direction_streaks;
        if (data_size > 2) {
            constexpr size_t desired_data_length = 50;
            const size_t past_data_size =
                desired_data_length < data_size
                    ? desired_data_length
                    : data_size;
            add_sequence(tree, data + 2, past_data_size);
        }
    }

    RawStockDataArray* future_stock_data_array;
    success = load_stock_data_from_year_from_disk(
        &future_stock_data_array, 2024);
    if (!success) {
        exit(1);
    }

    DirectionDataArray* future_direction_data;
    success = getDirectionData(
        &future_stock_data_array,
        &future_direction_data
    );
    if (!success) {
        exit(1);
    }
    DirectionStreakArray* future_direction_streak_array;
    success = getDirectionStreaks(
        &future_direction_data,
        &future_direction_streak_array
    );
    if (!success) {
        exit(1);
    }

    run_simulation(
        tree,
        future_stock_data_array,
        future_direction_streak_array
    );


    // For finding the min and max streaks; useful for tuning

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


    // Graphs streaks as points where magnitude is streak length

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


    // Naive bayes; a really long time to compute

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


    // These are here for when valgrind is run;

    // they are just too slow to run during iterations
    // freeDirectionCounts(all_direction_counts);
    // free_direction_streaks(direction_streak_array);
    // freeDirectionData(all_direction_data);
    // freeAllStockData(raw_stock_data_array);
}

int main() {
#if IS_PARALLEL
    printOpenMPVersion();
#endif

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
