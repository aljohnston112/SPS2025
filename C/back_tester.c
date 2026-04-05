#include "back_tester.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal_config.h"
#include "ranks.h"
#include "sequence_counting_trie.h"
#include "tree_file_util.h"

#define MIN_DEPTH  1
#define MAX_DEPTH  12
#define NUM_YEARS  (END_YEAR - START_YEAR + 1)
#define NUM_DEPTHS (MAX_DEPTH - MIN_DEPTH + 1)

typedef struct {
    double buy_price;
    size_t buy_day;
    double sell_price;
    size_t sell_day;
    double shares;
} OpenTrade;

bool get_all_fixed_size_tries(FixedSizeTrie trees[NUM_YEARS][NUM_DEPTHS]) {
    FilePathList file_data;
    if (!get_all_files_paths_recursive(
            TREE_DATA_FOLDER,
            &file_data
        )
    ) {
        return false;
    }
    for (size_t i = 0; i < file_data.file_count; i++) {
        const char* file_path = file_data.file_paths[i];
        const char* lastSlashPosition = strrchr(file_path, '/');
        if (lastSlashPosition == NULL) {
            free_all_files_paths(&file_data);
            return false;
        }
        const char* file_name = lastSlashPosition + 1;
        // skip "tree_"
        file_name += 5;
        char* end;
        const uint16_t trie_year = (uint16_t)strtol(file_name, &end, 10);
        if (end == file_name) {
            free_all_files_paths(&file_data);
            perror("Failed to parse year");
            return false;
        }
        const char* lastUnderscorePosition = strrchr(file_path, '_');
        const uint16_t trie_depth =
            (uint16_t)strtol(lastUnderscorePosition + 1, &end, 10);
        if (end == lastUnderscorePosition + 1) {
            free_all_files_paths(&file_data);
            perror("Failed to parse depth");
            return false;
        }
        const size_t year_idx = (size_t)(trie_year - START_YEAR);
        const size_t depth_idx = (size_t)(trie_depth - MIN_DEPTH);
        trees[year_idx][depth_idx] = read_fixed_size_trie_from_file(file_path);
    }
    return true;
}

bool run_fast_backtest_loop(SymbolToRanksHashMap* all_stock_ranks) {
    FixedSizeTrie trees[NUM_YEARS][NUM_DEPTHS] = {NULL};
    if (!get_all_fixed_size_tries(trees)) {
        free_symbol_to_ranks_hash_map(all_stock_ranks);
        return false;
    }

    // Start with $1000
    constexpr double starting_capital = 1000.0;
    double capital = starting_capital;

    double money_used_to_make_profit = 0.0;
    double profit = 0.0;
    double money_currently_in_held_stocks = 0.0;

    // Track how many trades happen and how many profit
    size_t total_trades = 0;
    size_t n_profit = 0;

    // Track open trades

    // open_trades needs to be freed
    // -------------------------------------------------------------------------
    OpenTrade* open_trades =
        malloc(
            NUMBER_OF_STOCK_EXAMPLES *
            NUMBER_OF_STOCK_EXAMPLES *
            sizeof(OpenTrade)
        );
    if (open_trades == NULL) {
        free_symbol_to_ranks_hash_map(all_stock_ranks);
        perror("malloc failed");
        return false;
    }
    size_t num_open_trades = 0;

    // Find the stock with the max number of data points
    size_t max_days = 0;
    for (size_t j = 0; j < all_stock_ranks->count; j++) {
        const StockRanks* stock = all_stock_ranks->symbol_to_ranks[j];
        if (stock && stock->size > max_days) {
            max_days = stock->size;
        }
    }

    // Track when stocks are bailed

    // bailed needs to be freed
    // -------------------------------------------------------------------------
    bool* bailed = calloc(all_stock_ranks->count, sizeof(bool));
    if (bailed == NULL) {
        free_symbol_to_ranks_hash_map(all_stock_ranks);
        free(open_trades);
        perror("calloc failed");
        return false;
    }

    // bail_days needs to be freed
    // -------------------------------------------------------------------------
    size_t* bail_days = calloc(
        all_stock_ranks->count,
        sizeof(size_t)
    );
    if (bail_days == NULL) {
        free_symbol_to_ranks_hash_map(all_stock_ranks);
        free(open_trades);
        free(bailed);
        perror("calloc failed");
        return false;
    }

    // For every day, up to a day when there is still room to sell
    for (
        size_t current_day = DAYS_PER_DIFF;
        current_day < max_days - BUY_SELL_LAG - 1;
        current_day++
    ) {
        // Go through all stocks
        for (size_t j = 0; j < all_stock_ranks->count; j++) {
            const StockRanks* future_stock_ranks =
                all_stock_ranks->symbol_to_ranks[j];

            // If the stock has enough data
            if (future_stock_ranks &&
                future_stock_ranks->size >
                current_day + BUY_SELL_LAG + DAYS_PER_DIFF + 1
            ) {
                // Go through every day in the data minus room to check for a sale

                // Bail out if we decided to bail earlier
                // A stock is bailed if it did not follow the up pattern and lead to a loss
                if (bailed[j] && bail_days[j] != 0 &&
                    bail_days[j] <= current_day
                ) {
                    continue;
                }

                // Close trades that were calculated to be sold this day
                for (size_t k = 0; k < num_open_trades; k++) {
                    // The trade to close
                    auto const open_trade = open_trades[k];

                    // If it is to be sold today
                    if (open_trade.sell_day == current_day) {
                        const double sell_price = open_trade.sell_price;

                        // Money from selling the stock
                        const double money_from_stock_sold =
                            sell_price * open_trade.shares;

                        // Add it back to the capital pool
                        capital += money_from_stock_sold;

                        const double money_put_into_stock =
                            (open_trade.buy_price * open_trade.shares);
                        profit += (money_from_stock_sold -
                            money_put_into_stock);
                        money_used_to_make_profit += money_put_into_stock;
                        money_currently_in_held_stocks -= money_put_into_stock;
                        total_trades++;

                        // And remove it from memory
                        open_trades[k] = open_trades[--num_open_trades];
                        open_trades[num_open_trades + 1] =
                            (OpenTrade){
                                .buy_price = 0,
                                .buy_day = 0,
                                .sell_price = 0,
                                .sell_day = 0,
                                .shares = 0
                            };
                        k--;

                        // Print current capital
                        printf(
                            "Trade %zu after %lu days: "
                            "Buy at %.2f, Sell at %.2f, "
                            "Total Assets: %.2f \n",
                            total_trades,
                            open_trade.sell_day - open_trade.buy_day,
                            open_trade.buy_price,
                            sell_price,
                            capital + money_currently_in_held_stocks
                        );
                    }
                }

                // Crash the program if there is not enough memory
                if (num_open_trades >= NUMBER_OF_STOCK_EXAMPLES *
                    NUMBER_OF_STOCK_EXAMPLES
                ) {
                    free_symbol_to_ranks_hash_map(all_stock_ranks);
                    free(open_trades);
                    free(bailed);
                    free(bail_days);
                    fprintf(stderr, "Too many open trades!\n");
                    return future_stock_ranks;
                }

                if (future_stock_ranks->dates[current_day]->year < 2000) {
                    continue;
                }

                const uint16_t year_limit =
                    future_stock_ranks->dates[current_day]->year - 1;
                double prediction = 0.0;
                // #pragma omp parallel for collapse(2) reduction(+:prediction)
                for (uint16_t year = START_YEAR;
                     year < year_limit;
                     year++
                ) {
                    constexpr uint64_t depth = MAX_TRIE_DEPTH;
                    FixedSizeTrie trie =
                        trees[year - START_YEAR][depth - MIN_DEPTH];
                    if (!trie.start) {
                        continue;
                    }

                    // Get that data and ask the trie whether it thinks we should buy
                    const size_t sequence_length = current_day < depth
                                                       ? current_day
                                                       : depth;
                    const long* past_sequence =
                        &future_stock_ranks->rank_diffs[
                            current_day - sequence_length
                        ];
                    size_t depth_found = 0;
                    double prediction_found = 0.0;
                    get_prediction(
                        &trie,
                        past_sequence,
                        sequence_length,
                        &prediction_found,
                        &depth_found
                    );
                    prediction += prediction_found;
                }

                // If the tree says we should buy and there is room to sell
                if (prediction > PREDICTION_THRESHOLD) {
                    // Only consider stocks under some price
                    const double buy_price =
                        future_stock_ranks->high_per_day[current_day + 1];
                    // Assume default sell date
                    const uint64_t sell_day = 1 + current_day + BUY_SELL_LAG;
                    const double sell_price =
                        future_stock_ranks->low_per_day[sell_day];

                    // For tracking how many trades were a profit
                    if (sell_price > buy_price) {
                        n_profit++;
                    }

                    // Take a portion of the current capital and
                    // use it to buy the stock
                    const double spent = capital * 0.1;
                    open_trades[num_open_trades++] = (OpenTrade){
                        .buy_price = buy_price,
                        .buy_day = current_day + 1,
                        .sell_day = sell_day,
                        .shares = spent / buy_price,
                        .sell_price = sell_price
                    };
                    capital -= spent;
                    money_currently_in_held_stocks += spent;

                    // If the trade resulted in a loss, do not consider it again
                    if (sell_price < buy_price && !bailed[j]) {
                        printf(
                            "Bailing on stock: %s after loss of %.2f%%\n",
                            future_stock_ranks->stock_symbol,
                            (1 -((sell_price / buy_price))) * 100
                        );
                        bail_days[j] = sell_day;
                        bailed[j] = true;
                    }
                }
            }
        }
    }

    // bailed freed
    // -------------------------------------------------------------------------
    free(bailed);

    // bail_days freed
    // -------------------------------------------------------------------------
    free(bail_days);

    printf(
        "Backtest finished. \n"
        "    Total trades: %zu, \n"
        "    Capital: %.2f\n"
        "    Percent of trades that resulted in profit: %.2f\n"
        "    Profit: %.2f\n"
        "    Money used to make profit: %.2f\n"
        "    Money currently in held stocks: %.2f\n"
        "    Percent gain: %.2f\n",
        total_trades,
        capital,
        (double)n_profit / (double)total_trades,
        profit,
        money_used_to_make_profit,
        money_currently_in_held_stocks,
        capital / starting_capital
    );

    double current_stock_value = 0.0;
    for (size_t k = 0; k < num_open_trades; k++) {
        // The trade to close
        auto const open_trade = open_trades[k];

        const double sell_price = open_trade.sell_price;
        const double money_from_stock_sold = sell_price * open_trade.shares;

        current_stock_value += money_from_stock_sold;
        open_trades[k] = open_trades[--num_open_trades];
        k--;
    }

    printf(
        "    Current money in stocks: %.2f\n",
        current_stock_value
    );

    // open_trades freed
    // -------------------------------------------------------------------------
    free(open_trades);
    return total_trades;
}

bool run_fast_backtest_with_past_stock_data(
    const StockDataTables* past_stock_data_tables
) {
    // all_stock_ranks must be freed
    // -------------------------------------------------------------------------
    SymbolToRanksHashMap* all_stock_ranks =
        malloc(sizeof(SymbolToRanksHashMap));
    if (all_stock_ranks == NULL) {
        perror("malloc failed");
        return false;
    }
    all_stock_ranks->count = 0;
    if (!initialize_symbol_to_ranks_hash_map(
            past_stock_data_tables,
            all_stock_ranks
        )
    ) {
        free(all_stock_ranks);
        return false;
    }

    if (!rank_stocks_by_low(
            past_stock_data_tables,
            all_stock_ranks,
            DAYS_PER_DIFF,
            BUY_SELL_LAG
        )
    ) {
        free_symbol_to_ranks_hash_map(all_stock_ranks);
        return false;
    }

    if (!run_fast_backtest_loop(all_stock_ranks)) {
        return false;
    }

    // all_stock_ranks freed
    // -------------------------------------------------------------------------
    free_symbol_to_ranks_hash_map(all_stock_ranks);
    return true;
}

bool run_fast_backtest() {
    // past_stock_data_tables needs to be freed
    // -------------------------------------------------------------------------
    StockDataTables* past_stock_data_tables =
        malloc(sizeof(StockDataTables));
    if (past_stock_data_tables == NULL) {
        fprintf(stderr, "malloc failed");
        exit(EXIT_FAILURE);
    }
    memset(past_stock_data_tables, 0, sizeof(StockDataTables));
    past_stock_data_tables->tables = nullptr;
    past_stock_data_tables->table_count = 0;
    past_stock_data_tables->capacity = 0;

    uint16_t start_year = START_YEAR;
    uint16_t end_year = END_YEAR;
    if (!load_stock_data_from_disk(
            past_stock_data_tables,
            &start_year,
            &end_year,
            INTERMEDIATE_DATA_FOLDER
        )
    ) {
        free(past_stock_data_tables);
        return false;
    }

    if (!run_fast_backtest_with_past_stock_data(past_stock_data_tables)) {
        free_stock_data_tables(past_stock_data_tables);
        return false;
    }

    // past_stock_data_tables freed
    // -------------------------------------------------------------------------
    free_stock_data_tables(past_stock_data_tables);

    return true;
}

void run_backtest(
    const SequenceCountingTrie* yearly_tree,
    const SymbolToRanksHashMap* all_stock_ranks
) {
    // Start with $1000
    constexpr double starting_capital = 1000.0;
    double capital = starting_capital;

    double money_used_to_make_profit = 0.0;
    double profit = 0.0;
    double money_currently_in_held_stocks = 0.0;

    // Track how many trades happen and how many profit
    size_t total_trades = 0;
    size_t n_profit = 0;

    // Track open trades
    OpenTrade* open_trades =
        malloc(
            NUMBER_OF_STOCK_EXAMPLES *
            NUMBER_OF_STOCK_EXAMPLES *
            sizeof(OpenTrade)
        );
    if (open_trades == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    size_t num_open_trades = 0;

    // Find the stock with the max number of data points
    size_t max_days = 0;
    for (size_t j = 0; j < all_stock_ranks->count; j++) {
        const StockRanks* stock = all_stock_ranks->symbol_to_ranks[j];
        if (stock && stock->size > max_days) {
            max_days = stock->size;
        }
    }

    // Track when stocks are bailed
    bool* bailed = calloc(all_stock_ranks->count, sizeof(bool));
    if (bailed == NULL) {
        perror("calloc failed");
        exit(EXIT_FAILURE);
    }
    size_t* bail_days = calloc(
        all_stock_ranks->count,
        sizeof(size_t)
    );
    if (bail_days == NULL) {
        perror("calloc failed");
        exit(EXIT_FAILURE);
    }

    // For every day, up to a day when there is still room to sell
    for (
        size_t i = DAYS_PER_DIFF;
        i < max_days - BUY_SELL_LAG - 1;
        i++
    ) {
        // Go through all stocks
        for (size_t j = 0; j < all_stock_ranks->count; j++) {
            const StockRanks* future_stock_ranks =
                all_stock_ranks->symbol_to_ranks[j];

            // If the stock has enough data
            if (future_stock_ranks &&
                future_stock_ranks->size > BUY_SELL_LAG
            ) {
                // Go through every day in the data minus room to check for a sale

                // Bail out if we decided to bail earlier
                if (bailed[j] && bail_days[j] != 0 && bail_days[j] <= i) {
                    continue;
                }

                // Close trades that were calculated to be sold this day
                for (size_t k = 0; k < num_open_trades; k++) {
                    // The trade to close
                    auto const open_trade = open_trades[k];

                    // If it is to be sold today
                    if (open_trade.sell_day == i) {
                        const double sell_price = open_trade.sell_price;

                        // Money from selling the stock
                        const double money_from_stock_sold =
                            sell_price * open_trade.shares;

                        // Add it back to the capital pool
                        capital += money_from_stock_sold;

                        const double money_put_into_stock =
                            (open_trade.buy_price * open_trade.shares);
                        profit += (money_from_stock_sold -
                            money_put_into_stock);
                        money_used_to_make_profit += money_put_into_stock;
                        money_currently_in_held_stocks -= money_put_into_stock;
                        total_trades++;

                        // And remove it from memory
                        open_trades[k] = open_trades[--num_open_trades];
                        k--;

                        // Print current capital
                        // printf(
                        //     "Trade %zu after %lu days: Buy at %.2f, Sell at %.2f, Total Capital: %.2f \n",
                        //     total_trades,
                        //     open_trade.sell_day - open_trade.buy_day,
                        //     open_trade.buy_price,
                        //     sell_price,
                        //     capital
                        // );
                    }
                }

                // Crash the program if there is not enough memory
                if (num_open_trades >= NUMBER_OF_STOCK_EXAMPLES *
                    NUMBER_OF_STOCK_EXAMPLES) {
                    fprintf(stderr, "Too many open trades!\n");
                    exit(1);
                }

                // The tree can only use so much data
                const size_t sequence_length =
                    i < MAX_TRIE_DEPTH ? i : MAX_TRIE_DEPTH;

                // Get that data and ask the tree whether it thinks we should buy
                const long* past_sequence =
                    &future_stock_ranks->rank_diffs[0 + (i - sequence_length)];
                double prediction = 0.0;
                size_t depth = 0;
                get_prediction_from_trie(
                    yearly_tree,
                    past_sequence,
                    sequence_length,
                    &prediction,
                    &depth
                );

                // If the tree says we should buy and there is room to sell
                if (prediction > PREDICTION_THRESHOLD &&
                    i + 1 + BUY_SELL_LAG < future_stock_ranks->size
                ) {
                    // Only consider stocks under some price
                    const double buy_price =
                        future_stock_ranks->high_per_day[i + 1];
                    if (buy_price < 1) {
                        // Assume default sell date
                        uint64_t sell_day = 1 + i + BUY_SELL_LAG;
                        double sell_price = -1.0;

                        // But see if we can sell earlier for profit,
                        // hold until profit,
                        // or wait til the stock reaches last stock day
                        for (size_t day = i + 2;
                             day < future_stock_ranks->size;
                             day++
                        ) {
                            if (future_stock_ranks->low_per_day[day] >=
                                buy_price * 2
                            ) {
                                sell_price =
                                    future_stock_ranks->low_per_day[day];
                                sell_day = day;
                                break;
                            }
                        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                        if (sell_price == -1.0) {
#pragma GCC diagnostic pop
                            sell_price = future_stock_ranks->low_per_day[
                                future_stock_ranks->size - 1
                            ];
                            sell_day = future_stock_ranks->size - 1;
                        }

                        // For tracking how many trades were a profit
                        if (sell_price > buy_price) {
                            n_profit++;
                        }

                        if (j % 1000 == 0) {
                            printf(
                                "    Percent of trades that resulted in profit: %.2f\n",
                                (double)n_profit / (double)total_trades
                            );
                        }

                        // Take a portion of the current capital and
                        // use it to buy the stock
                        const double spent = capital * 0.1;
                        open_trades[num_open_trades++] = (OpenTrade){
                            .buy_price = buy_price,
                            .buy_day = i + 1,
                            .sell_day = sell_day,
                            .shares = spent / buy_price,
                            .sell_price = sell_price
                        };
                        capital -= spent;
                        money_currently_in_held_stocks += spent;

                        // If the trade resulted in a loss, do not consider it again
                        if (sell_price < buy_price && !bailed[j]) {
                            printf(
                                "%s\n",
                                future_stock_ranks->stock_symbol
                                //"Bailing on stock: %s after selling for %.2f\n",
                                // stock_ranks->stock_symbol,
                                // (sell_price / buy_price)
                            );
                            bail_days[j] = sell_day;
                            bailed[j] = true;
                        }
                    }
                }
            }
        }
    }

    free(bailed);
    free(bail_days);

    printf(
        "Backtest finished. \n"
        "    Total trades: %zu, \n"
        "    Capital: %.2f\n"
        "    Percent of trades that resulted in profit: %.2f\n"
        "    Profit: %.2f\n"
        "    Money used to make profit: %.2f\n"
        "    Money currently in held stocks: %.2f\n"
        "    Percent gain: %.2f\n",
        total_trades,
        capital,
        (double)n_profit / (double)total_trades,
        profit,
        money_used_to_make_profit,
        money_currently_in_held_stocks,
        capital / starting_capital
    );

    double current_stock_value = 0.0;
    for (size_t k = 0; k < num_open_trades; k++) {
        // The trade to close
        auto const open_trade = open_trades[k];

        const double sell_price = open_trade.sell_price;

        // Money from selling the stock
        const double money_from_stock_sold =
            sell_price * open_trade.shares;

        // Add it back to the capital pool
        current_stock_value += money_from_stock_sold;

        // And remove it from memory
        open_trades[k] = open_trades[--num_open_trades];
        k--;
    }

    printf(
        "    Current money in stocks: %.2f\n",
        current_stock_value
    );

    free(open_trades);
}

bool create_future_diffs_and_run_back_test(
    const SequenceCountingTrie* tree,
    const StockDataTables* future_stock_data_tables
) {
    // future_stock_ranks needs to be freed
    // -------------------------------------------------------------------------
    SymbolToRanksHashMap* future_stock_ranks =
        malloc(sizeof(SymbolToRanksHashMap));
    if (future_stock_ranks == NULL) {
        perror("malloc failed");
        return false;
    }
    future_stock_ranks->count = 0;

    if (!initialize_symbol_to_ranks_hash_map(
            future_stock_data_tables,
            future_stock_ranks
        )
    ) {
        free(future_stock_ranks);
        return false;
    }

    if (!rank_stocks_by_low(
            future_stock_data_tables,
            future_stock_ranks,
            DAYS_PER_DIFF,
            BUY_SELL_LAG
        )
    ) {
        free_symbol_to_ranks_hash_map(future_stock_ranks);
        return false;
    }

    run_backtest(
        tree,
        future_stock_ranks
    );

    // future_stock_ranks freed
    // -------------------------------------------------------------------------
    free_symbol_to_ranks_hash_map(future_stock_ranks);

    return true;
}

bool load_future_data_then_create_future_diffs_and_run_back_test(
    const SequenceCountingTrie* tree
) {
    // future_stock_data_tables needs to be freed
    // -------------------------------------------------------------------------
    StockDataTables* future_stock_data_tables =
        malloc(sizeof(StockDataTables));
    if (future_stock_data_tables == NULL) {
        perror("malloc failed");
        return false;
    }
    memset(future_stock_data_tables, 0, sizeof(StockDataTables));
    future_stock_data_tables->tables = nullptr;
    future_stock_data_tables->table_count = 0;
    future_stock_data_tables->capacity = 0;

    if (!load_stock_data_from_disk(
        future_stock_data_tables,
        &future_start_year,
        &future_end_year,
        INTERMEDIATE_DATA_FOLDER
    )) {
        free(future_stock_data_tables);
        return false;
    }

    if (!create_future_diffs_and_run_back_test(
            tree,
            future_stock_data_tables
        )
    ) {
        free(future_stock_data_tables);
        return false;
    }

    // future_stock_data_tables freed
    // -------------------------------------------------------------------------
    free_stock_data_tables(future_stock_data_tables);

    return true;
}

bool create_past_tree_then_create_future_diffs_and_run_back_test(
    const SymbolToRanksHashMap* past_symbol_to_ranks_map
) {
    // trie needs to be freed
    // -------------------------------------------------------------------------
    SequenceCountingTrie* trie = create_sequence_counting_trie(0);
    fill_trie(
        past_symbol_to_ranks_map,
        trie,
        DAYS_PER_DIFF,
        BUY_SELL_LAG,
        MAX_TRIE_DEPTH
    );
    // print_tree(tree);
    if (!load_future_data_then_create_future_diffs_and_run_back_test(trie)) {
        free_trie(trie);
        return false;
    }

    // trie freed
    // -------------------------------------------------------------------------
    free_trie(trie);

    return true;
}

void print_stats(const SymbolToRanksHashMap* past_symbol_to_ranks_map) {
    // (the diff size) + (the day after_diff) + (the price lag)
    constexpr size_t required_data_size = (DAYS_PER_DIFF + 1) + (1) + (
        BUY_SELL_LAG);

    size_t sum_of_subsequences = 0;
    size_t sum_data_points = 0;
    size_t max_data_size = 0;
    for (size_t j = 0; j < RANK_MAP_SIZE; j++) {
        const StockRanks* stock_ranks =
            past_symbol_to_ranks_map->symbol_to_ranks[j];
        if (stock_ranks) {
            const size_t data_size = stock_ranks->size;
            if (data_size > max_data_size) {
                max_data_size = data_size;
            }
            sum_of_subsequences +=
                data_size + (MAX_TRIE_DEPTH * (data_size - MAX_TRIE_DEPTH + 1));
            sum_data_points += data_size;
        }
    }

    printf("Number of subsequences: %lu\n", sum_of_subsequences);
    printf("Max data size: %lu\n", max_data_size);
    printf("Number of data points: %lu\n", sum_data_points);

    int64_t min_rank_diff = INT64_MAX;
    int64_t max_rank_diff = INT64_MIN;
    for (size_t j = 0; j < RANK_MAP_SIZE; j++) {
        const StockRanks* stock_ranks =
            past_symbol_to_ranks_map->symbol_to_ranks[j];
        if (stock_ranks) {
            const size_t data_size = stock_ranks->size;
            if (data_size >= required_data_size) {
                for (size_t i = DAYS_PER_DIFF;
                     // (the data size) - (the day after a diff) - (the price lag)
                     i < data_size - 1 - BUY_SELL_LAG;
                     i++
                ) {
                    const int64_t rank_diff = stock_ranks->rank_diffs[i];
                    if (rank_diff < min_rank_diff) {
                        min_rank_diff = rank_diff;
                    } else if (rank_diff > max_rank_diff) {
                        max_rank_diff = rank_diff;
                    }
                }
            }
        }
    }
    printf("Min: %ld\n", min_rank_diff);
    printf("Max: %ld\n", max_rank_diff);
}

bool create_diffs_and_run_back_test(
    const StockDataTables* past_stock_data_tables
) {
    // past_symbol_to_ranks_map must be freed
    // -------------------------------------------------------------------------
    SymbolToRanksHashMap* past_symbol_to_ranks_map =
        malloc(sizeof(SymbolToRanksHashMap));
    if (past_symbol_to_ranks_map == NULL) {
        perror("malloc failed");
        return false;
    }
    past_symbol_to_ranks_map->count = 0;
    if (!initialize_symbol_to_ranks_hash_map(
            past_stock_data_tables,
            past_symbol_to_ranks_map
        )
    ) {
        free_symbol_to_ranks_hash_map(past_symbol_to_ranks_map);
        return false;
    }

    if (!rank_stocks_by_low(
            past_stock_data_tables,
            past_symbol_to_ranks_map,
            DAYS_PER_DIFF,
            BUY_SELL_LAG
        )
    ) {
        free_symbol_to_ranks_hash_map(past_symbol_to_ranks_map);
        return false;
    }

    print_stats(past_symbol_to_ranks_map);

    if (!create_past_tree_then_create_future_diffs_and_run_back_test(
            past_symbol_to_ranks_map
        )
    ) {
        free_symbol_to_ranks_hash_map(past_symbol_to_ranks_map);
        return false;
    }

    // past_symbol_to_ranks_map freed
    // -------------------------------------------------------------------------
    free_symbol_to_ranks_hash_map(past_symbol_to_ranks_map);

    return true;
}

__attribute__((malloc)) StockDataTables* calloc_stock_data_tables(void) {
    StockDataTables* past_stock_data_tables =
        calloc(1, sizeof(StockDataTables));
    if (past_stock_data_tables == nullptr) {
        perror("malloc failed");
        return nullptr;
    }
    return past_stock_data_tables;
}

bool prepare_data_and_run_back_test() {
    // past_stock_data_tables needs to be freed
    // -------------------------------------------------------------------------
    StockDataTables* past_stock_data_tables = calloc_stock_data_tables();

    if (!load_stock_data_from_disk(
            past_stock_data_tables,
            &past_start_year,
            &past_end_year,
            INTERMEDIATE_DATA_FOLDER
        )
    ) {
        free(past_stock_data_tables);
        return false;
    }

    // if (!create_diffs_and_run_back_test(past_stock_data_tables)) {
    //     free_stock_data_tables(past_stock_data_tables);
    //     return false;
    // }

    // past_stock_data_tables freed
    // -------------------------------------------------------------------------
    free_stock_data_tables(past_stock_data_tables);

    return true;
}
