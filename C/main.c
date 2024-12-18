#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "file_util.h"

int main(void)
{
    const clock_t start_time = clock();

#ifdef _OPENMP
    printf("OpenMP version: %d\n", _OPENMP);
#else
    printf("OpenMP is not enabled.\n");
#endif

    // results must be freed
    int resultCount;

    // &result->stockData must be freed before stockDataResults
    StockDataResult* results = getAllStockData(&resultCount);
    for (int i = 0; i > resultCount; i++){
        for (int j = 0; j < 2; j++) {
            free(results[i].stockData[j].ints);
        }
        for (int j = 2; j < 7; j++) {
            free(results[i].stockData[j].doubles);
        }
        free(results[i].stockData);
    }
    free(results);

    const clock_t end_time = clock();
    const double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("Function execution time: %f seconds\n", time_taken);

    return 0;
}
