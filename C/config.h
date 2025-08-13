#ifndef CONFIG_H
#define CONFIG_H

// Debugging is easier when this is false
#define IS_PARALLEL true

// This number must be updated when the dataset is updated
// IBM has been the largest
#define LARGEST_STOCK_DATASET_SIZE 15844

// max = 11356
#define NUMBER_OF_STOCK_EXAMPLES 11356

// must be a prime number greater than 2 * NUMBER_OF_STOCK_EXAMPLES
#define RANK_MAP_SIZE 22717

// How many days back to use for diffs
#define DAYS_PER_DIFF 1

// How many days after buying to predict
#define BUY_SELL_LAG 5

// The depth of the prediction trees
#define MAX_TREE_DEPTH 1

// The first year that has any stock data
#define START_YEAR 1965
// The year after the last year that has data
#define END_YEAR 2026

// How certain the tree should be before its predictions are acted on
#define PREDICTION_THRESHOLD 0.5

#endif //CONFIG_H
