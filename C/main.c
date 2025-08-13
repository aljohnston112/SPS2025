#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <bits/fcntl-linux.h>
#include <sys/time.h>

#include "file_util.h"
#include "config.h"
#include "internal_config.h"
#include "ranks.h"
#include "sequence_counting_trie.h"
#include "tree_file_util.h"
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

constexpr uint16_t past_end_year = 2023;
constexpr uint16_t past_start_year = past_end_year - 2;

constexpr u_int16_t future_start_year = past_end_year;
constexpr u_int16_t future_end_year = future_start_year + 1;

void print_promising_stocks(
    const SequenceCountingTrie* yearly_tree,
    const SymbolToRanksHashMap* all_stock_ranks
) {
    constexpr size_t required_data_size =
        (DAYS_PER_DIFF + 1) + (1) + (BUY_SELL_LAG);
    printf("Promising stocks:\n");

    for (size_t j = 0; j < all_stock_ranks->count; j++) {
        const StockRanks* stock_ranks = all_stock_ranks->symbol_to_ranks[j];
        if (!stock_ranks || stock_ranks->capacity < required_data_size) {
            continue;
        }
        const size_t sequence_length =
            stock_ranks->capacity < MAX_TREE_DEPTH
                ? stock_ranks->capacity
                : MAX_TREE_DEPTH;

        const long* past_sequence = &stock_ranks->rank_diffs[
            stock_ranks->capacity - sequence_length
        ];

        double prediction = 0.0;
        size_t depth = 0;
        get_prediction_from_trie(
            yearly_tree,
            past_sequence,
            sequence_length,
            &prediction,
            &depth
        );

        if (prediction > PREDICTION_THRESHOLD && stock_ranks->low_per_day[
            sequence_length - 1] < 0.75) {
            printf(
                "%s\n",
                stock_ranks->stock_symbol
            );
        }
    }
}

void create_future_diffs_and_print_promising_stocks(
    const SequenceCountingTrie* tree,
    const StockDataTables* future_stock_data_tables
) {
    // future_stock_ranks needs to be freed
    // -------------------------------------------------------------------------
    SymbolToRanksHashMap* future_stock_ranks =
        malloc(sizeof(SymbolToRanksHashMap));
    if (future_stock_ranks == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    future_stock_ranks->count = 0;

    initialize_symbol_to_ranks_hash_map(
        future_stock_data_tables,
        future_stock_ranks
    );

    rank_stocks_by_low(
        future_stock_data_tables,
        future_stock_ranks,
        DAYS_PER_DIFF,
        BUY_SELL_LAG
    );

    print_promising_stocks(
        tree,
        future_stock_ranks
    );

    // future_stock_ranks freed
    // -------------------------------------------------------------------------
    free_symbol_to_ranks_hash_map(future_stock_ranks);
}

void load_future_data_then_create_future_diffs_and_print_promising_stocks(
    const SequenceCountingTrie* tree
) {
    // future_stock_data_tables needs to be freed
    // -------------------------------------------------------------------------
    StockDataTables* future_stock_data_tables =
        malloc(sizeof(StockDataTables));
    if (future_stock_data_tables == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    memset(future_stock_data_tables, 0, sizeof(StockDataTables));
    future_stock_data_tables->tables = nullptr;
    future_stock_data_tables->table_count = 0;
    future_stock_data_tables->capacity = 0;

    constexpr uint16_t start_year = 2025;
    const bool success = load_stock_data_from_disk(
        future_stock_data_tables,
        &start_year,
        nullptr,
        INTERMEDIATE_DATA_FOLDER
    );
    if (!success) {
        exit(1);
    }

    create_future_diffs_and_print_promising_stocks(
        tree,
        future_stock_data_tables
    );

    // future_stock_data_tables freed
    // -------------------------------------------------------------------------
    free_stock_data_tables(future_stock_data_tables);
}

/**
 * Adds rank_diff sequences and whether they lead to profit (went_up)
 * (BUY_SELL_LAG + 1) days after the diff sequence.
 * All stocks and subsequences will be added to the tree.
 *
 * @param symbol_to_ranks_map A map of stock symbols to rank data.
 *                            The rank data must include rank_diffs
 *                            and went_up data.
 * @param tree The tree to feed the rank_diffs sequence data to.
 */
void fill_tree(
    const SymbolToRanksHashMap* symbol_to_ranks_map,
    SequenceCountingTrie* tree
) {
    // (the diff size) + (the day after_diff) + (the price lag)
    constexpr size_t required_data_size =
        (DAYS_PER_DIFF + 1) + (1) + (BUY_SELL_LAG);

    for (size_t j = 0; j < RANK_MAP_SIZE; j++) {
        const StockRanks* stock_ranks =
            symbol_to_ranks_map->symbol_to_ranks[j];
        if (stock_ranks) {
            const size_t data_size = stock_ranks->capacity;
            const size_t desired_data_length = data_size;
            const size_t actual_data_size = data_size < desired_data_length
                                                ? data_size
                                                : desired_data_length;
            if (data_size >= required_data_size) {
                for (size_t i = DAYS_PER_DIFF;
                     i < actual_data_size - BUY_SELL_LAG - 1;
                     i++
                ) {
                    add_sequence_to_trie(
                        tree,
                        stock_ranks->rank_diffs + i,
                        // 1 and buy_sell_lag are because no data is generated past that
                        // 1 is for the gap day between when a diff is seen and the buy/sell
                        actual_data_size - i - 1 - BUY_SELL_LAG,
                        stock_ranks->went_up + i,
                        MAX_TREE_DEPTH
                    );
                }
            }
        }
    }
}

void create_past_tree_then_create_future_diffs_and_print_promising_stocks(
    const SymbolToRanksHashMap* past_symbol_to_ranks_map
) {
    // tree needs to be freed
    // -------------------------------------------------------------------------
    SequenceCountingTrie* tree = create_sequence_counting_trie(0);
    fill_tree(past_symbol_to_ranks_map, tree);
    load_future_data_then_create_future_diffs_and_print_promising_stocks(tree);

    // tree freed
    // -------------------------------------------------------------------------
    free_trie(tree);
}

void create_diffs_and_print_promising_stocks(
    const StockDataTables* past_stock_data_tables
) {
    // past_symbol_to_ranks_map must be freed
    // -------------------------------------------------------------------------
    SymbolToRanksHashMap* past_symbol_to_ranks_map =
        malloc(sizeof(SymbolToRanksHashMap));
    if (past_symbol_to_ranks_map == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    past_symbol_to_ranks_map->count = 0;
    initialize_symbol_to_ranks_hash_map(
        past_stock_data_tables,
        past_symbol_to_ranks_map
    );

    rank_stocks_by_low(
        past_stock_data_tables,
        past_symbol_to_ranks_map,
        DAYS_PER_DIFF,
        BUY_SELL_LAG
    );


    create_past_tree_then_create_future_diffs_and_print_promising_stocks(
        past_symbol_to_ranks_map
    );

    // past_symbol_to_ranks_map freed
    // -------------------------------------------------------------------------
    free_symbol_to_ranks_hash_map(past_symbol_to_ranks_map);
}

void process_and_print_promising_stocks(void) {
    // past_stock_data_tables needs to be freed
    // -------------------------------------------------------------------------
    StockDataTables* past_stock_data_tables =
        malloc(sizeof(StockDataTables));
    if (past_stock_data_tables == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    memset(past_stock_data_tables, 0, sizeof(StockDataTables));
    past_stock_data_tables->tables = nullptr;
    past_stock_data_tables->table_count = 0;
    past_stock_data_tables->capacity = 0;

    constexpr uint16_t start = 2024;
    constexpr uint16_t end = 2025;
    const bool success = load_stock_data_from_disk(
        past_stock_data_tables,
        &start,
        &end,
        INTERMEDIATE_DATA_FOLDER
    );
    if (!success) {
        exit(1);
    }
    create_diffs_and_print_promising_stocks(past_stock_data_tables);

    // past_stock_data_tables freed
    // -------------------------------------------------------------------------
    free_stock_data_tables(past_stock_data_tables);
}

typedef struct {
    double buy_price;
    size_t buy_day;
    double sell_price;
    size_t sell_day;
    double shares;
} OpenTrade;

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
        if (stock && stock->capacity > max_days) {
            max_days = stock->capacity;
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
                future_stock_ranks->capacity > BUY_SELL_LAG
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
                    i < MAX_TREE_DEPTH ? i : MAX_TREE_DEPTH;

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
                    i + 1 + BUY_SELL_LAG < future_stock_ranks->capacity
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
                             day < future_stock_ranks->capacity;
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
                                future_stock_ranks->capacity - 1
                            ];
                            sell_day = future_stock_ranks->capacity - 1;
                        }

                        // For tracking how many trades were a profit
                        if (sell_price > buy_price) {
                            n_profit++;
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

void create_future_diffs_and_run_back_test(
    const SequenceCountingTrie* tree,
    const StockDataTables* future_stock_data_tables
) {
    // future_stock_ranks needs to be freed
    // -------------------------------------------------------------------------
    SymbolToRanksHashMap* future_stock_ranks =
        malloc(sizeof(SymbolToRanksHashMap));
    if (future_stock_ranks == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    future_stock_ranks->count = 0;

    initialize_symbol_to_ranks_hash_map(
        future_stock_data_tables,
        future_stock_ranks
    );

    rank_stocks_by_low(
        future_stock_data_tables,
        future_stock_ranks,
        DAYS_PER_DIFF,
        BUY_SELL_LAG
    );

    run_backtest(
        tree,
        future_stock_ranks
    );

    // future_stock_ranks freed
    // -------------------------------------------------------------------------
    free_symbol_to_ranks_hash_map(future_stock_ranks);
}

void load_future_data_then_create_future_diffs_and_run_back_test(
    const SequenceCountingTrie* tree
) {
    // future_stock_data_tables needs to be freed
    // -------------------------------------------------------------------------
    StockDataTables* future_stock_data_tables =
        malloc(sizeof(StockDataTables));
    if (future_stock_data_tables == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    memset(future_stock_data_tables, 0, sizeof(StockDataTables));
    future_stock_data_tables->tables = nullptr;
    future_stock_data_tables->table_count = 0;
    future_stock_data_tables->capacity = 0;

    const bool success = load_stock_data_from_disk(
        future_stock_data_tables,
        &future_start_year,
        &future_end_year,
        INTERMEDIATE_DATA_FOLDER
    );
    if (!success) {
        exit(1);
    }

    create_future_diffs_and_run_back_test(tree, future_stock_data_tables);

    // future_stock_data_tables freed
    // -------------------------------------------------------------------------
    free_stock_data_tables(future_stock_data_tables);
}

void create_past_tree_then_create_future_diffs_and_run_back_test(
    const SymbolToRanksHashMap* past_symbol_to_ranks_map
) {
    // tree needs to be freed
    // -------------------------------------------------------------------------
    SequenceCountingTrie* tree = create_sequence_counting_trie(0);
    fill_tree(past_symbol_to_ranks_map, tree);
    // print_tree(tree);
    load_future_data_then_create_future_diffs_and_run_back_test(tree);

    // tree freed
    // -------------------------------------------------------------------------
    free_trie(tree);
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
            const size_t data_size = stock_ranks->capacity;
            if (data_size > max_data_size) {
                max_data_size = data_size;
            }
            sum_of_subsequences +=
                data_size + (MAX_TREE_DEPTH * (data_size - MAX_TREE_DEPTH + 1));
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
            const size_t data_size = stock_ranks->capacity;
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

void create_diffs_and_run_back_test(
    const StockDataTables* past_stock_data_tables
) {
    // past_symbol_to_ranks_map must be freed
    // -------------------------------------------------------------------------
    SymbolToRanksHashMap* past_symbol_to_ranks_map =
        malloc(sizeof(SymbolToRanksHashMap));
    if (past_symbol_to_ranks_map == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    past_symbol_to_ranks_map->count = 0;
    initialize_symbol_to_ranks_hash_map(
        past_stock_data_tables,
        past_symbol_to_ranks_map
    );

    rank_stocks_by_low(
        past_stock_data_tables,
        past_symbol_to_ranks_map,
        DAYS_PER_DIFF,
        BUY_SELL_LAG
    );

    print_stats(past_symbol_to_ranks_map);

    create_past_tree_then_create_future_diffs_and_run_back_test(
        past_symbol_to_ranks_map
    );

    // past_symbol_to_ranks_map freed
    // -------------------------------------------------------------------------
    free_symbol_to_ranks_hash_map(past_symbol_to_ranks_map);
}

void prepare_data_and_run_back_test() {
    // past_stock_data_tables needs to be freed
    // -------------------------------------------------------------------------
    StockDataTables* past_stock_data_tables =
        malloc(sizeof(StockDataTables));
    if (past_stock_data_tables == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    memset(past_stock_data_tables, 0, sizeof(StockDataTables));
    past_stock_data_tables->tables = nullptr;
    past_stock_data_tables->table_count = 0;
    past_stock_data_tables->capacity = 0;

    const bool success = load_stock_data_from_disk(
        past_stock_data_tables,
        &past_start_year,
        &past_end_year,
        INTERMEDIATE_DATA_FOLDER
    );
    if (!success) {
        exit(1);
    }
    create_diffs_and_run_back_test(past_stock_data_tables);

    // past_stock_data_tables freed
    // -------------------------------------------------------------------------
    free_stock_data_tables(past_stock_data_tables);
}

void save_yearly_trees(void) {
    // past_stock_data_tables needs to be freed
    // -------------------------------------------------------------------------
    StockDataTables* past_stock_data_tables =
        malloc(sizeof(StockDataTables));
    if (past_stock_data_tables == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    memset(past_stock_data_tables, 0, sizeof(StockDataTables));
    past_stock_data_tables->tables = nullptr;
    past_stock_data_tables->table_count = 0;
    past_stock_data_tables->table_count = 0;

    // first year is 1964
    u_int16_t start_year = 1964;
    uint16_t end_year = start_year + 1;
    while (start_year < 2025) {
        start_year++;
        end_year++;

        char* filename;
        asprintf(
            &filename,
            "%s/tree_%d_%d_%d_%d.tree",
            TREE_DATA_FOLDER,
            start_year,
            DAYS_PER_DIFF,
            BUY_SELL_LAG,
            MAX_TREE_DEPTH
        );

        if (access(filename, F_OK) == 0) {
            // file exists
            free(filename);
            continue;
        }

        const bool success = load_stock_data_from_disk(
            past_stock_data_tables,
            &start_year,
            &end_year,
            INTERMEDIATE_DATA_FOLDER
        );
        if (!success) {
            exit(1);
        }

        // past_symbol_to_ranks_map must be freed
        // ---------------------------------------------------------------------
        SymbolToRanksHashMap* past_symbol_to_ranks_map =
            malloc(sizeof(SymbolToRanksHashMap));
        if (past_symbol_to_ranks_map == NULL) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }
        past_symbol_to_ranks_map->count = 0;

        initialize_symbol_to_ranks_hash_map(
            past_stock_data_tables,
            past_symbol_to_ranks_map
        );

        rank_stocks_by_low(
            past_stock_data_tables,
            past_symbol_to_ranks_map,
            DAYS_PER_DIFF,
            BUY_SELL_LAG
        );

        // tree needs to be freed
        // ---------------------------------------------------------------------
        SequenceCountingTrie* tree = create_sequence_counting_trie(0);
        assert(tree);
        fill_tree(past_symbol_to_ranks_map, tree);


        FILE* fd = fopen(filename, "w+");
        free(filename);
        if (fd == NULL) {
            perror("fopen failed");
            exit(1);
        }
        export_tree_to_file(tree, fd);
        fclose(fd);

        // tree freed
        // ---------------------------------------------------------------------
        free_trie(tree);

        // past_symbol_to_ranks_map freed
        // ---------------------------------------------------------------------
        free_symbol_to_ranks_hash_map(past_symbol_to_ranks_map);

        free_stock_data_tables(past_stock_data_tables);
        past_stock_data_tables = malloc(sizeof(StockDataTables));
        if (past_stock_data_tables == NULL) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }
        memset(past_stock_data_tables, 0, sizeof(StockDataTables));
        past_stock_data_tables->tables = nullptr;
        past_stock_data_tables->table_count = 0;
        past_stock_data_tables->capacity = 0;
    }

    if (past_stock_data_tables) {
        free_stock_data_tables(past_stock_data_tables);
    }
}

void process(void) {
    // process_and_print_promising_stocks();
    prepare_data_and_run_back_test();
    // save_yearly_trees();
    // print_bounds_on_trees();
    // TreeHashMap* map = load_trees_from_year(1965);
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
