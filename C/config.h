#ifndef CONFIG_H
#define CONFIG_H

#define IS_PARALLEL true

#define LARGEST_STOCK_DATASET_SIZE 15844

// max = 11356
#define NUMBER_OF_STOCK_EXAMPLES 11356

// must be a prime number greater than 2 * NUMBER_OF_STOCK_EXAMPLES
#define RANK_MAP_SIZE 22717


#define BUY_SELL_LAG 5
#define DAYS_PER_DIFF 1
#define MAX_TREE_DEPTH 10

#define PREDICTION_THRESHOLD 0.67
#endif //CONFIG_H
