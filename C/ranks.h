#ifndef RANKS_H
#define RANKS_H

#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "file_util.h"


typedef struct {
    char* stock_symbol;
    int64_t* rank_per_day;
    double* low_per_day;
    double* high_per_day;
    const Date** dates;
    int64_t* rank_diffs;
    bool* went_up;
    size_t current_index;
    size_t data_size;
} StockRanks;

typedef struct {
    StockRanks* symbol_to_ranks[RANK_MAP_SIZE];
    size_t count;
} SymbolToRanksHashMap;

typedef struct {
    StockDataTable* table;
    size_t current_index;
} ActiveStock;

void free_symbol_to_ranks_hash_map(const SymbolToRanksHashMap* ranks);

void create_rank_diffs(
    const SymbolToRanksHashMap* symbol_to_ranks_map,
    size_t days_per_diff,
    size_t buy_sell_lag
);

bool is_leap_year(uint16_t year);

void next_day(Date* date);

uint16_t hash_symbol(const char* symbol);

StockRanks* get_from_ranks_hash_map(
    const SymbolToRanksHashMap* map,
    const char* stock_symbol
);

int compare_by_low(const void* a, const void* b);

bool is_date_less_or_equal(const Date* date1, const Date* date2);

int compare_by_date(const void* a, const void* b);

bool is_date_less(const Date* date1,const Date* date2);

void get_min_and_max_dates(
    StockDataTable** stock_data_tables,
    size_t table_count,
    Date* min_date,
    Date* max_date
);

void rank_valid_stocks_by_low(
    const SymbolToRanksHashMap* all_stock_ranks,
    StockDataTable** valid_stock_tables,
    size_t valid_stock_count
);

void rank_stocks_by_low(
    const StockDataTables* stock_data_tables,
    const SymbolToRanksHashMap* all_stock_ranks,
    size_t days_per_diff,
    size_t buy_sell_lag
);

void add_to_rank_hash_map(
    SymbolToRanksHashMap* symbol_to_ranks_map,
    StockRanks* stock_ranks
);

void initialize_symbol_to_ranks_hash_map(
    const StockDataTables* stock_data_tables,
    SymbolToRanksHashMap* symbol_to_ranks_hash_map
);

#endif //RANKS_H
