#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "file_util.h"

int main(void)
{
struct timeval start, stop;
gettimeofday(&start, NULL);

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

    gettimeofday(&stop, NULL);
    const double time_taken = (double)(stop.tv_usec - start.tv_usec) / 1000000 + (double)(stop.tv_sec - start.tv_sec);
    printf("Function execution time: %f seconds\n", time_taken);

    return 0;
}
