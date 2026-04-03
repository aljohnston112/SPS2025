#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// Debugging is easier when this is false
#define IS_PARALLEL true

// This number must be updated when the dataset is updated
// IBM has been the largest
#define LARGEST_STOCK_DATASET_SIZE 15844

// max = 11356
#define NUMBER_OF_STOCK_EXAMPLES 11356
#define RANK_MAP_SIZE 22717
static_assert(
    RANK_MAP_SIZE > (2 * NUMBER_OF_STOCK_EXAMPLES),
    "Load factor too high! Quadratic probing requires RANK_MAP_SIZE must be "
    "a prime number greater than 2 * NUMBER_OF_STOCK_EXAMPLES"
);

#define NUMBER_OF_COLUMNS_IN_CSV 10

// How many days back to use for diffs
#define DAYS_PER_DIFF 1

// How many days after buying to predict
#define BUY_SELL_LAG 5

// The depth of the prediction trees
#define MAX_TRIE_DEPTH 100

// The first year that has any stock data
#define START_YEAR 1965
// The year after the last year that has data
#define END_YEAR 2026

// How certain the tree should be before its predictions are acted on
#define PREDICTION_THRESHOLD 0.5

constexpr uint16_t past_end_year = 2023;
constexpr uint16_t past_start_year = past_end_year - 1;

constexpr uint16_t future_start_year = past_end_year;
constexpr uint16_t future_end_year = future_start_year + 1;

#endif //CONFIG_H
