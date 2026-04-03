#include "stock_picker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal_config.h"
#include "ranks.h"
#include "sequence_counting_trie.h"

void print_promising_stocks(
    const SequenceCountingTrie* yearly_tree,
    const SymbolToRanksHashMap* all_stock_ranks
) {
    constexpr size_t required_data_size =
        (DAYS_PER_DIFF + 1) + (1) + (BUY_SELL_LAG);
    printf("Promising stocks:\n");

    for (size_t j = 0; j < all_stock_ranks->count; j++) {
        const StockRanks* stock_ranks = all_stock_ranks->symbol_to_ranks[j];
        if (!stock_ranks || stock_ranks->size < required_data_size) {
            continue;
        }
        const size_t sequence_length =
            stock_ranks->size < MAX_TRIE_DEPTH
                ? stock_ranks->size
                : MAX_TRIE_DEPTH;

        const long* past_sequence = &stock_ranks->rank_diffs[
            stock_ranks->size - sequence_length
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

bool create_future_diffs_and_print_promising_stocks(
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
        free_symbol_to_ranks_hash_map(future_stock_ranks);
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

    print_promising_stocks(
        tree,
        future_stock_ranks
    );

    // future_stock_ranks freed
    // -------------------------------------------------------------------------
    free_symbol_to_ranks_hash_map(future_stock_ranks);

    return true;
}

bool load_future_data_then_create_future_diffs_and_print_promising_stocks(
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

    uint16_t start_year = 2025;
    if (!load_stock_data_from_disk(
            future_stock_data_tables,
            &start_year,
            nullptr,
            INTERMEDIATE_DATA_FOLDER
        )
    ) {
        free_stock_data_tables(future_stock_data_tables);
        return false;
    }

    if (!create_future_diffs_and_print_promising_stocks(
            tree,
            future_stock_data_tables
        )
    ) {
        free_stock_data_tables(future_stock_data_tables);
        return false;
    }

    // future_stock_data_tables freed
    // -------------------------------------------------------------------------
    free_stock_data_tables(future_stock_data_tables);

    return true;
}


bool create_past_tree_then_create_future_diffs_and_print_promising_stocks(
    const SymbolToRanksHashMap* past_symbol_to_ranks_map
) {
    // tree needs to be freed
    // -------------------------------------------------------------------------
    SequenceCountingTrie* trie = create_sequence_counting_trie(0);
    fill_trie(
        past_symbol_to_ranks_map,
        trie,
        DAYS_PER_DIFF,
        BUY_SELL_LAG,
        MAX_TRIE_DEPTH
    );
    if (!load_future_data_then_create_future_diffs_and_print_promising_stocks(
            trie
        )
    ) {
        free_trie(trie);
        return false;
    }

    // tree freed
    // -------------------------------------------------------------------------
    free_trie(trie);

    return true;
}

bool create_diffs_and_print_promising_stocks(
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


    if (!create_past_tree_then_create_future_diffs_and_print_promising_stocks(
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

bool process_and_print_promising_stocks() {
    // past_stock_data_tables needs to be freed
    // -------------------------------------------------------------------------
    StockDataTables* past_stock_data_tables =
        malloc(sizeof(StockDataTables));
    if (past_stock_data_tables == NULL) {
        fprintf(stderr, "malloc failed");
        return false;
    }
    memset(past_stock_data_tables, 0, sizeof(StockDataTables));
    past_stock_data_tables->tables = nullptr;
    past_stock_data_tables->table_count = 0;
    past_stock_data_tables->capacity = 0;

    uint16_t start = 2024;
    uint16_t end = 2025;
    if (!load_stock_data_from_disk(
            past_stock_data_tables,
            &start,
            &end,
            INTERMEDIATE_DATA_FOLDER
        )
    ) {
        free_stock_data_tables(past_stock_data_tables);
        return false;
    }
    if (!create_diffs_and_print_promising_stocks(past_stock_data_tables)) {
        free_stock_data_tables(past_stock_data_tables);
        return false;
    }

    // past_stock_data_tables freed
    // -------------------------------------------------------------------------
    free_stock_data_tables(past_stock_data_tables);

    return true;
}
