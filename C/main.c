#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <sys/time.h>

#include "file_util.h"
#include "directions.h"
#include "config.h"
#include "ranks.h"
#include "tree.h"
#include "../CPP/grapher.h"

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
    AllStockDataArrays* past_stock_data_array;

    constexpr u_int16_t start_year = 2023;
    constexpr u_int16_t end_year = 2024;
    const bool success = load_stock_data_from_disk(
        &past_stock_data_array,
        nullptr,
        &end_year
    );
    if (!success) {
        exit(1);
    }

    AllStockRanks* all_stock_ranks;
    create_all_stock_ranks(
        past_stock_data_array,
        &all_stock_ranks
    );

    rank_by_low(
        past_stock_data_array,
        all_stock_ranks
    );

    convert_and_plot(all_stock_ranks);


    // AllDirectionDataArrays* past_direction_data;
    // success = getDirectionData(
    //     &past_stock_data_array,
    //     &past_direction_data
    // );
    // if (!success) {
    //     exit(1);
    // }
    // AllProfitStreakArrays* past_direction_streak_array;
    // success = getProfitStreaks(
    //     &past_direction_data,
    //     &past_direction_streak_array
    // );
    // if (!success) {
    //     exit(1);
    // }

    // HashMap* tree = create_hash_map(0);
    // for (int i = 0; i < past_direction_streak_array->data_size; i++) {
    //     const ProfitStreakArray* direction_streak_row_array =
    //         &past_direction_streak_array->profit_streaks_arrays[i];
    //     const size_t data_size = direction_streak_row_array->data_size;
    //
    //     const long* data = direction_streak_row_array->profit_streak_array;
    //     if (data_size > 2) {
    //         const size_t desired_data_length = data_size;
    //         // Skip the first 2 since the first is a boundary
    //         // and the second is a negative number
    //         // The tree root starts with a positive number
    //         add_sequence(tree, data + 2, desired_data_length - 2);
    //     }
    // }

    // AllStockDataArrays* future_stock_data_array;
    //
    // constexpr u_int16_t future_end_year = 2024;
    // success = load_stock_data_from_disk(
    //     &future_stock_data_array,
    //     &future_end_year,
    //     nullptr
    // );
    // if (!success) {
    //     exit(1);
    // }
    //
    // AllDirectionDataArrays* future_direction_data;
    // success = getDirectionData(
    //     &future_stock_data_array,
    //     &future_direction_data
    // );
    // if (!success) {
    //     exit(1);
    // }
    // AllProfitStreakArrays* future_profit_streak_array;
    // success = getProfitStreaks(
    //     &future_direction_data,
    //     &future_profit_streak_array
    // );
    // if (!success) {
    //     exit(1);
    // }

    // print_tree(tree);
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
