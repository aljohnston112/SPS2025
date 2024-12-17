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

    int resultCount;
    StockDataResult* results = getAllStockData(&resultCount);
    free(results);
    return 0;
}
