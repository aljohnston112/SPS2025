#include "rank_hash_map.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Allocates space for the rank data in the given symbol_to_ranks_hash_map.
 * This must be freed by the caller using free_symbol_to_ranks_hash_map.
 *
 * @param stock_data_tables The stock data to be used
 *                          to determine how much space is needed
 *                          in the symbol_to_ranks_hash_map.
 * @param symbol_to_ranks_hash_map The SymbolToRanksHashMap to initialize.
 */
void initialize_symbol_to_ranks_hash_map(
    const StockDataTables* stock_data_tables,
    SymbolToRanksHashMap* symbol_to_ranks_hash_map
) {
    symbol_to_ranks_hash_map->count = 0;
    memset(
        symbol_to_ranks_hash_map->symbol_to_ranks,
        0,
        sizeof(symbol_to_ranks_hash_map->symbol_to_ranks)
    );

    // * 2 to fill in missing days; i.e. weekends or unreported days
    constexpr size_t number_of_rows = LARGEST_STOCK_DATASET_SIZE * 2;
    const size_t number_of_stocks = stock_data_tables->table_count;
    for (size_t i = 0; i < number_of_stocks; ++i) {
        const StockDataTable* table = &stock_data_tables->tables[i];

        StockRanks* stock_ranks = malloc(sizeof(StockRanks));
        if (stock_ranks == NULL) {
            fprintf(stderr, "Failed to allocate memory for stock_ranks");
            exit(EXIT_FAILURE);
        }
        stock_ranks->stock_symbol = table->stock_symbol;
        stock_ranks->rank_per_day = calloc(
            number_of_rows,
            sizeof(size_t)
        );
        if (stock_ranks->rank_per_day == NULL) {
            fprintf(
                stderr,
                "Failed to allocate memory for stock_ranks->rank_per_day"
            );
            exit(EXIT_FAILURE);
        }
        stock_ranks->low_per_day = calloc(
            number_of_rows,
            sizeof(double)
        );
        if (stock_ranks->low_per_day == NULL) {
            fprintf(
                stderr,
                "Failed to allocate memory for stock_ranks->low_per_day"
            );
            exit(EXIT_FAILURE);
        }
        stock_ranks->high_per_day = calloc(
            number_of_rows,
            sizeof(double)
        );
        if (stock_ranks->high_per_day == NULL) {
            fprintf(
                stderr,
                "Failed to allocate memory for stock_ranks->high_per_day"
            );
            exit(EXIT_FAILURE);
        }
        stock_ranks->dates = calloc(
            number_of_rows,
            sizeof(Date*)
        );
        if (stock_ranks->dates == NULL) {
            fprintf(
                stderr,
                "Failed to allocate memory for stock_ranks->dates"
            );
            exit(EXIT_FAILURE);
        }
        stock_ranks->current_index = 0;
        stock_ranks->capacity = number_of_rows;
        // rank_diffs and went_up are initialized after generating ranks
        stock_ranks->rank_diffs = nullptr;
        stock_ranks->went_up = nullptr;
        add_to_rank_hash_map(
            symbol_to_ranks_hash_map,
            stock_ranks
        );
    }
}

/**
 * Adds the given stock ranks to the given hash map.
 *
 * @param symbol_to_ranks_map The map to add the stack ranks to.
 * @param stock_ranks The stock ranks to add to the map.
 */
void add_to_rank_hash_map(
    SymbolToRanksHashMap* symbol_to_ranks_map,
    StockRanks* stock_ranks
) {
    const char* string_key = stock_ranks->stock_symbol;
    const uint16_t key = hash_symbol(string_key);

    size_t i = 0;
    // quadratic probing
    size_t index = (key + (i * i)) % RANK_MAP_SIZE;
    while (symbol_to_ranks_map->symbol_to_ranks[index] != NULL) {
        // slot is occupied
        i++;
        index = (key + (i * i)) % RANK_MAP_SIZE;
    }
    assert(symbol_to_ranks_map->symbol_to_ranks[index] == NULL);
    symbol_to_ranks_map->symbol_to_ranks[index] = stock_ranks;
    symbol_to_ranks_map->count++;
    assert(symbol_to_ranks_map->count < RANK_MAP_SIZE / 2);
}

/**
 * Hashes the given symbol.
 *
 * @param symbol The symbol to hash.
 * @return The hash of the given symbol.
 */
uint16_t hash_symbol(const char* symbol) {
    // Hash was over 2 times faster than gpref in production testing
    uint16_t hash = 0;
    for (int i = 0; symbol[i]; i++) {
        hash = (hash << 5) ^ (uint16_t)(symbol[i] - 'a');
    }
    return hash % RANK_MAP_SIZE;
}

/**
 * Tries to get the rank series for the given stock symbol.
 * If the value is not in the map, it will return null.
 *
 * @param map The map.
 * @param stock_symbol The stock symbol.
 * @return The rank series for the stock if it is in the map, else null.
 */
StockRanks* get_from_ranks_hash_map(
    const SymbolToRanksHashMap* map,
    const char* stock_symbol
) {
    size_t i = 0;
    const long key = hash_symbol(stock_symbol);
    while (i < RANK_MAP_SIZE) {
        const size_t index = ((size_t)key + (i * i)) % RANK_MAP_SIZE;
        StockRanks* stock_ranks = map->symbol_to_ranks[index];
        if (stock_ranks == NULL) {
            return nullptr;
        }
        if (stock_ranks->stock_symbol == stock_symbol) {
            return stock_ranks;
        }
        i++;
    }
    assert(false);
}

/**
 * Frees every stock ranks' allocated memory and the hash map itself.
 * @param symbol_to_ranks_hash_map
 */
void free_symbol_to_ranks_hash_map(
    SymbolToRanksHashMap* symbol_to_ranks_hash_map
) {
    for (size_t i = 0; i < RANK_MAP_SIZE; ++i) {
        StockRanks* sr = symbol_to_ranks_hash_map->symbol_to_ranks[i];
        if (sr) {
            free(sr->rank_per_day);
            free(sr->low_per_day);
            free(sr->high_per_day);
            free(sr->dates);
            free(sr->rank_diffs);
            free(sr->went_up);
            free(sr);
        }
    }
    free(symbol_to_ranks_hash_map);
}
