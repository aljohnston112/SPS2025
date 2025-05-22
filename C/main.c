#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
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
    const AllDirectionDataArrays* all_direction_data,
    const AllDirectionCountArrays* all_direction_counts
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
        int localBoughtOnLoss = 0;
        int localBoughtOnProfit = 0;
        int localTotal = 0;
        const DirectionDataArray* direction_data =
            &all_direction_data->direction_data_arrays[i];
        for (int k = 0; k < direction_data->data_size; k++) {
            const u_int8_t full_row = direction_data->direction_data_array[k];
            const u_int8_t row_without_profit =
                full_row & PROFIT_REMOVAL_BITMASK;
            const bool was_profit = full_row >> WAS_PROFIT_POSITION;

            const u_int8_t upRow =
                row_without_profit + PROFIT_EXTRACTION_BITMASK;
            const u_int8_t downRow =
                row_without_profit & PROFIT_REMOVAL_BITMASK;

            double up_probability_log = 0.0;
            double down_probability_log = 0.0;

            for (int j = 0; j < number_of_stocks; j++) {
                const DirectionCountArray* direction_counts =
                    &all_direction_counts->direction_counts_array[j];
                const double up_votes =
                    direction_counts->direction_count_array[upRow];
                const double down_votes =
                    direction_counts->direction_count_array[downRow];
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
                    ++localBoughtOnLoss;
                } else {
                    ++localBoughtOnProfit;
                }
            }
            ++localTotal;
        }
        const int sum = localBoughtOnLoss + localBoughtOnProfit;
        if (sum != 0) {
            printf("Bought on loss: %f \n",
                   (double)localBoughtOnLoss / (double)sum);
            printf("Bought on profit: %f \n",
                   (double)localBoughtOnProfit / (double)sum);
        }
        boughtOnLoss += localBoughtOnLoss;
        boughtOnProfit += localBoughtOnProfit;
        total += localTotal;
    }

    const int sum = boughtOnLoss + boughtOnProfit;
    printf("Bought on loss: %f \n", (double)boughtOnLoss / (double)sum);
    printf("Bought on profit: %f \n", (double)boughtOnProfit / (double)sum);
}

void run_simulation(
    const HashMap* tree,
    const AllStockDataArrays* future_stock_data_array,
    const AllProfitStreakArrays* future_profit_streak_arrays
) {
    double profit = 1.0;

    for (size_t i = 0; i < future_profit_streak_arrays->data_size; i++) {
        const RowArray* row_array = &future_stock_data_array->row_arrays[i];
        const Row* stock_data_rows = row_array->rows;

        const ProfitStreakArray* profit_streak_arrays =
            future_profit_streak_arrays->profit_streaks_arrays;
        const ProfitStreakArray* profit_streak_array = &profit_streak_arrays[i];

        assert(
            strcmp(
                row_array->stock_symbol,
                profit_streak_array->stock_symbol
            ) == 0
        );

        // Skip the 1st streak, since it is on a boundary
        // Then the 2nd since it a negative streak
        // Since the tree's root starts at positive streaks,
        // the 3rd streak needs to be skipped so it can be used for predicting
        // when the 4th streak will end
        const size_t number_of_profit_steaks = profit_streak_array->data_size;
        if (number_of_profit_steaks > 3) {
            double buy_price = 0;
            bool bought = false;

            const long* streak_array = profit_streak_array->profit_streak_array;

            // +3 so the current day is the day after a streak reversal
            u_int64_t current_day = SELL_LAG + 3;

            // Skip the 1st three streaks
            for (size_t j = 0; j < 3; j++) {
                const long streak = streak_array[j];
                const size_t streak_length = streak > 0 ? streak : -streak;
                current_day += streak_length;
            }

            assert(streak_array[3] <= 0);
            for (size_t current_streak_index = 4;
                 current_streak_index < number_of_profit_steaks;
                 current_streak_index++
            ) {
                const long current_streak = streak_array[current_streak_index];

                // Current day is the day after the steak reversed
                // A streak of length 1 ends the day before the reversal is noticed
                const size_t streak_length =
                    current_streak < 0
                        ? -current_streak
                        : current_streak;
                const size_t current_streak_reverse_index =
                    current_day + streak_length - 1;

                double prediction;
                get_prediction_from_hash_map(
                    tree,
                    streak_array + 2, // Start from the 3rd streak
                    current_streak_index - 3,
                    &prediction
                );
                const size_t buy_sell_prediction = floor(
                    prediction < 0 ? -prediction : prediction
                );

                // Current day is the day after the last streak reversed
                // Since the streak started yesterday,
                // need -1 to go back to the previous day when the streak ended
                // Also need -1 since a streak of 1 day shouldn't add any days
                const size_t predicted_reverse_day =
                    buy_sell_prediction - 1 + current_day - 1;

                // Need to make sure the sell day is in bounds of the stock data
                if (predicted_reverse_day < row_array->data_size) {
                    // Want to buy only if we are on a negative streak
                    // to catch the positive on the way back up
                    // Want to sell only if we are on a positive streak
                    // to miss out on the drop
                    if ((!bought && current_streak < 0) ||
                        (bought && current_streak > 0)
                    ) {
                        // We should bail if we have stock,
                        // and we noticed the stock started a down streak
                        // despite what the prediction says
                        const bool streak_ended =
                            bought &&
                            predicted_reverse_day >
                            current_streak_reverse_index + 1;

                        // Only buy if the predicted reverse happens one day out
                        // since we need to buy this day, and sell the next
                        if (!bought &&
                            predicted_reverse_day > current_day + 1
                        ) {
                            if (predicted_reverse_day <
                                current_streak_reverse_index
                            ) {
                                printf("Bought early\n");
                                buy_price =
                                    stock_data_rows[predicted_reverse_day].high;
                            } else if (predicted_reverse_day >
                                current_streak_reverse_index
                            ) {
                                printf("Bought late\n");
                                buy_price =
                                    stock_data_rows[current_streak_reverse_index
                                        + 1].high;
                            } else {
                                printf("Bought on time\n");
                                buy_price =
                                    stock_data_rows[predicted_reverse_day].high;
                            }
                            bought = true;
                        }

                        // Sell if streak ended early and price went up
                        if (bought) {
                            if (streak_ended) {
                                const double actual_low =
                                    stock_data_rows[current_day].low;
                                if (actual_low > buy_price) {
                                    bought = false;
                                    profit *= actual_low / buy_price;
                                    printf("Profit : %f \n", profit);
                                }
                            } else {
                                if (predicted_reverse_day <
                                    current_streak_reverse_index
                                ) {
                                    printf("Sold early\n");
                                    const double sell_price =
                                        stock_data_rows[
                                            predicted_reverse_day + SELL_LAG + 1
                                        ].low;
                                    profit *= sell_price / buy_price;
                                    printf("Profit : %f \n", profit);
                                } else if (predicted_reverse_day >
                                    current_streak_reverse_index
                                ) {
                                    printf("Sold late\n");
                                    const double sell_price =
                                        stock_data_rows[
                                            current_streak_index + SELL_LAG + 1
                                        ].low;
                                    profit *= sell_price / buy_price;
                                    printf("Profit : %f \n", profit);
                                } else {
                                    printf("Sold on time\n");
                                    const double sell_price =
                                        stock_data_rows[
                                            predicted_reverse_day + SELL_LAG + 1
                                        ].low;
                                    profit *= sell_price / buy_price;
                                    printf("Profit : %f \n", profit);
                                }
                                bought = false;
                            }
                        }
                    }
                }
                current_day +=
                    current_streak < 0
                        ? -current_streak
                        : current_streak;
            }
        }
    }
}

void process(void) {
    AllStockDataArrays* past_stock_data_array;

    constexpr u_int16_t start_year = 2023;
    constexpr u_int16_t end_year = 2024;
    bool success = load_stock_data_from_disk(
        &past_stock_data_array,
        &start_year,
        &end_year
    );
    if (!success) {
        exit(1);
    }

    AllDirectionDataArrays* past_direction_data;
    success = getDirectionData(&past_stock_data_array,
                               &past_direction_data);
    if (!success) {
        exit(1);
    }
    AllProfitStreakArrays* past_direction_streak_array;
    success = getProfitStreaks(
        &past_direction_data,
        &past_direction_streak_array
    );
    if (!success) {
        exit(1);
    }

    HashMap* tree = create_hash_map(0);
    for (int i = 0; i < past_direction_streak_array->data_size; i++) {
        const ProfitStreakArray* direction_streak_row_array =
            &past_direction_streak_array->profit_streaks_arrays[i];
        const size_t data_size = direction_streak_row_array->data_size;

        const long* data = direction_streak_row_array->profit_streak_array;
        if (data_size > 2) {
            const size_t desired_data_length = data_size;
            // Skip the first 2 since the first is a boundary
            // and the second is a negative number
            // The tree root starts with a positive number
            add_sequence(tree, data + 2, desired_data_length - 2);
        }
    }

    AllStockDataArrays* future_stock_data_array;

    constexpr u_int16_t future_end_year = 2024;
    success = load_stock_data_from_disk(
        &future_stock_data_array,
        &future_end_year,
        nullptr
    );
    if (!success) {
        exit(1);
    }

    AllDirectionDataArrays* future_direction_data;
    success = getDirectionData(
        &future_stock_data_array,
        &future_direction_data
    );
    if (!success) {
        exit(1);
    }
    AllProfitStreakArrays* future_profit_streak_array;
    success = getProfitStreaks(
        &future_direction_data,
        &future_profit_streak_array
    );
    if (!success) {
        exit(1);
    }


    size_t n_correct = 0;
    size_t n_wrong = 0;

    FILE* p = popen("gnuplot -persistent", "w");
    fprintf(p, "set title 'Actual vs Predicted'\n");
    fprintf(p, "set xlabel 'Actual'\n");
    fprintf(p, "set ylabel 'Predicted'\n");
    fprintf(p, "plot '-' with points pointtype 7 title 'Data'\n");
    for (size_t i = 0; i < future_profit_streak_array->data_size; i++) {
        const ProfitStreakArray* direction_streak_array =
            &future_profit_streak_array->profit_streaks_arrays[i];
        if (direction_streak_array->data_size > 4) {
            for (size_t j = 0; j < direction_streak_array->data_size - 3; j++) {
                double prediction;
                get_prediction_from_hash_map(
                    tree,
                    direction_streak_array->profit_streak_array + 2,
                    j + 1,
                    &prediction
                );
                if (prediction != 0.0) {
                    const long actual = direction_streak_array->profit_streak_array[
                        j + 3];
                    printf("%ld vs %f\n",
                           direction_streak_array->profit_streak_array[j + 3],
                           prediction
                    );
                    fprintf(p, "%ld %f\n", actual, prediction);
                }
            }
        }
    }


    run_simulation(
        tree,
        future_stock_data_array,
        future_profit_streak_array
    );

    // print_tree(tree);

    // For finding the min and max streaks; useful for tuning

    // long max = 0;
    // long min = 0;
    // for (int i = 0; i < past_direction_streak_array->data_size; i++) {
    //     const DirectionStreakRowArray* direction_streak_row_array =
    //         &past_direction_streak_array->direction_streaks_arrays[i];
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
    // for (int i = 0; i < past_direction_streak_array->data_size; i++) {
    //     fprintf(p, "set terminal qt %d\n", i);
    //     fprintf(p, "set title 'Direction Streaks %d'\n", i);
    //     fprintf(
    //         p,
    //         "plot '-' with points pointtype 7 pointsize 0.5 title 'Direction Streaks'\n");
    //
    //     int x = 0;
    //     const DirectionStreakRowArray* direction_streak_row_array =
    //         &past_direction_streak_array->direction_streaks_arrays[i];
    //     const long* data = direction_streak_row_array->direction_streaks;
    //     const u_long data_size = direction_streak_row_array->data_size;
    //     for (u_long k = 0; k < data_size; k++) {
    //         fprintf(p, "%d %ld\n", x++, data[k]);
    //     }
    //     // fprintf(p, "e\n");
    // }


    // Naive bayes; a really long time to compute

    // DirectionCountsArray* past_direction_counts;
    // success = calculateDirectionCounts(&past_direction_data,
    //                                    &past_direction_counts);
    // if (!success) {
    //     exit(1);
    // }
    //
    // performNaiveBayesOnEntireStockHistoryDirectionBits(
    //     past_direction_data,
    //     past_direction_counts
    // );

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
