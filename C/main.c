#include <stdio.h>
#include <sys/time.h>

#include "file_util.h"
#include "probability.h"

void printOpenMPVersion() {
#ifdef _OPENMP
  printf("OpenMP version: %d\n", _OPENMP);
#else
  printf("OpenMP is not enabled.\n");
#endif
}

void process(void) {
  // results must be freed
  int resultsCount;
  RawStockDataResults* results = loadAllStockDataFromDisk(&resultsCount);

  DirectionData* data = malloc(sizeof(DirectionData) * resultsCount);
  getDirectionData(results, resultsCount, data);

// TODO free direction data

  freeAllStockData(results, resultsCount);
  // results freed
  // ---------------------------------------------------------------------------------------------------
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
