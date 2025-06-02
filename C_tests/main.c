#include <stdlib.h>

#include  "csv_util_test.h"
#include "file_util_test.h"
#include "ranks_test.h"
#include "tree_test.h"

int main() {
    run_file_util_tests();
    run_csv_util_tests();
    run_ranks_tests();
    run_tree_test();
    return EXIT_SUCCESS;
}
