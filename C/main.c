#include <stdio.h>
#include <sys/time.h>

#include "file_util.h"
#include "probability.h"
#include "config.h"
#include "internal_config.h"

void printOpenMPVersion() {
#ifdef _OPENMP
    printf("OpenMP version: %d\n", _OPENMP);
#else
  printf("OpenMP is not enabled.\n");
#endif
}

void process(void) {
    // results must be freed ---------------------------------------------------------------------------------------------
    int number_of_stocks;
    RawStockDataResults* all_stock_data = loadAllStockDataFromDisk(&number_of_stocks);

    DirectionData* all_direction_data = malloc(sizeof(DirectionData) * number_of_stocks);
    if (all_direction_data == NULL) {
        freeAllStockData(all_stock_data, number_of_stocks);
        perror("malloc failed");
        return;
    }
    for (int i = 0; i < number_of_stocks; i++) {
        all_direction_data[i].directionData = malloc(all_stock_data[i].data_size * sizeof(u_int8_t));
        if (all_direction_data[i].directionData == NULL) {
            for (int j = 0; j < i; j++) {
                free(all_direction_data[j].directionData);
            }
            free(all_direction_data);
            freeAllStockData(all_stock_data, number_of_stocks);
            perror("malloc failed");
            return;
        }
    }

    getDirectionData(all_stock_data, number_of_stocks, all_direction_data);

    DirectionCounts* all_direction_counts = malloc(sizeof(DirectionCounts) * number_of_stocks);
    if (all_direction_counts == NULL) {
        for (int j = 0; j < number_of_stocks; j++) {
            free(all_direction_data[j].directionData);
        }
        free(all_direction_data);
        freeAllStockData(all_stock_data, number_of_stocks);
        perror("malloc failed");
        return;
    }
    calculate_direction_counts(all_direction_data, number_of_stocks, all_direction_counts);

    int boughtOnLoss = 0;
    int boughtOnProfit = 0;
    int total = 0;

#pragma omp parallel for default(none) shared(all_direction_data, number_of_stocks, all_direction_counts, boughtOnProfit, boughtOnLoss, total) num_threads(180) proc_bind(close) if(IS_PARALLEL)
    for (int i = number_of_stocks - 100; i < number_of_stocks; i++) {
        const DirectionData* direction_data = &all_direction_data[i];
        for (int k = 0; k < direction_data->dataSize; k++) {
            const u_int8_t row = direction_data->directionData[k] & ~(1 << WAS_PROFIT_POSITION);
            bool was_profit = false;
            if (((direction_data->directionData[k] >> WAS_PROFIT_POSITION) & 1) == 1) {
                was_profit = true;
            }

            const u_int8_t upRow = row + (1 << WAS_PROFIT_POSITION);
            const u_int8_t downRow = row & ~(1 << WAS_PROFIT_POSITION);

            int up_votes = 0;
            int down_votes = 0;
            for (int j = 0; j < number_of_stocks; j++) {
                const DirectionCounts* direction_counts = &all_direction_counts[j];
                up_votes += direction_counts->directionCounts[upRow];
                down_votes += direction_counts->directionCounts[downRow];
            }
            const double vote_ratio = (double)up_votes / (double)down_votes;
            //printf("Up to down ratio: %f \n", vote_ratio);
            if (vote_ratio > 1 && !was_profit) {
#pragma omp critical
                {
                    ++boughtOnLoss;
                }
            }
            if (vote_ratio > 1 && was_profit) {
#pragma omp critical
                {
                    ++boughtOnProfit;
                }
            }
#pragma omp critical
            {
                ++total;
            }
        }
    }

    printf("Bought on loss: %f \n", (double)boughtOnLoss / (double)total);
    printf("Bought on profit: %f \n", (double)boughtOnProfit / (double)total);

    free(all_direction_counts);
    for
    (
        int i = 0;
        i < number_of_stocks;
        i
        ++
    ) {
        free(all_direction_data[i].directionData);
    }
    free(all_direction_data);
    freeAllStockData(all_stock_data, number_of_stocks);
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
