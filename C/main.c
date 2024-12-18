#include <omp.h>
#include <stdio.h>

#include "file_util.h"

int main(void)
{
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
        free(results[i].stockData);
    }
    free(results);

    return 0;
}
