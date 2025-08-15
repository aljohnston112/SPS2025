#include <sys/time.h>

#include "back_tester.h"
#include "ranks.h"
#include "stock_picker.h"
#include "tree_file_util.h"

#if IS_PARALLEL

void printOpenMPVersion() {
#ifdef _OPENMP
    printf("OpenMP version: %d\n", _OPENMP);
#else
    printf("OpenMP is not enabled.\n");
#endif
}
#endif

void process(void) {
    // process_and_print_promising_stocks();
    prepare_data_and_run_back_test();
    // save_yearly_trees();
    // print_bounds_on_tries();
    // FixedSizeTrie map = load_trie_from_year(1965);
    // run_fast_backtest();
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
