#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <sys/time.h>

#include "file_util.h"
#include "directions.h"
#include "config.h"
#include "internal_config.h"

void printOpenMPVersion() {
#ifdef _OPENMP
    printf("OpenMP version: %d\n", _OPENMP);
#else
  printf("OpenMP is not enabled.\n");
#endif
}

void performNaiveBayesOnEntireStockHistoryDirectionDits(
    const DirectionDataArray* all_direction_data,
    const DirectionCountsArray* all_direction_counts
) {
    const size_t number_of_stocks = all_direction_data->data_size;
    int boughtOnLoss = 0;
    int boughtOnProfit = 0;
    int total = 0;

#pragma omp parallel for default(none) shared(stderr, all_direction_data, number_of_stocks, all_direction_counts) reduction(+:boughtOnLoss, boughtOnProfit, total) num_threads(12) proc_bind(close) if(IS_PARALLEL)
    for (int i = 0; i < number_of_stocks; i++) {
        const DirectionDataRowArray* direction_data = &all_direction_data->direction_data_arrays[i];
        for (int k = 0; k < direction_data->data_size; k++) {
            const u_int8_t row = direction_data->direction_data[k] & ~(1 << WAS_PROFIT_POSITION);
            bool was_profit = false;
            if (((direction_data->direction_data[k] >> WAS_PROFIT_POSITION) & 1) == 1) {
                was_profit = true;
            }

            const u_int8_t upRow = row + (1 << WAS_PROFIT_POSITION);
            const u_int8_t downRow = row & ~(1 << WAS_PROFIT_POSITION);

            double up_probability_log = 0.0;
            double down_probability_log = 0.0;

            for (int j = 0; j < number_of_stocks; j++) {
                const DirectionCounts* direction_counts = &all_direction_counts->direction_counts[j];
                const double up_votes = direction_counts->direction_counts[upRow];
                const double down_votes = direction_counts->direction_counts[downRow];
                const double sum = up_votes + down_votes;

                if (sum > 0.0) {
                    if (up_votes > 0) {
                        const double up_ratio = log((double)up_votes / (double)sum);
                        if (up_probability_log > DBL_MAX - up_ratio) {
                            fprintf(stderr, "Overflow detected in up_probability_log\n");
                            exit(EXIT_FAILURE);
                        }
                        up_probability_log += up_ratio;
                    }
                    if (down_votes > 0) {
                        const double down_ratio = log((double)down_votes / (double)sum);
                        if (down_probability_log > DBL_MAX - down_ratio) {
                            fprintf(stderr, "Overflow detected in down_probability_log\n");
                            exit(EXIT_FAILURE);
                        }
                        down_probability_log += down_ratio;
                    }
                }
            }

            if (up_probability_log > down_probability_log) {
                if (!was_profit) {
                    ++boughtOnLoss;
                }
                else {
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
    // raw_stock_data_array must be freed ---------------------------------------------------------------------------------------------
    RawStockDataArray raw_stock_data_array;
    bool success = loadAllStockDataFromDisk(&raw_stock_data_array);
    if (!success) {
        return;
    }

    DirectionDataArray all_direction_data;
    success = getDirectionData(&raw_stock_data_array, &all_direction_data);
    if (!success) {
        freeAllStockData(&raw_stock_data_array);
        return;
    }

    DirectionCountsArray all_direction_counts;
    success = calculateDirectionCounts(&all_direction_data, &all_direction_counts);
    if (!success) {
        freeDirectionData(&all_direction_data);
        freeAllStockData(&raw_stock_data_array);
        return;
    }

    performNaiveBayesOnEntireStockHistoryDirectionDits(&all_direction_data, &all_direction_counts);

    freeDirectionCounts(&all_direction_counts);
    freeDirectionData(&all_direction_data);
    freeAllStockData(&raw_stock_data_array);
    // results freed -----------------------------------------------------------------------------------------------------
}

int main() {
    // printOpenMPVersion();

    struct timeval start, stop;
    gettimeofday(&start, NULL);

    process();

    gettimeofday(&stop, NULL);
    const double time_taken = (double)(stop.tv_usec - start.tv_usec) / 1000000 +
        (double)(stop.tv_sec - start.tv_sec);
    printf("Function execution time: %f seconds\n", time_taken);

    return 0;
}
