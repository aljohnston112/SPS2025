#include "directions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "file_util.h"
#include "internal_config.h"


StockPriceDirectionTables* get_price_direction_tables(
    const StockDataTables* stock_data_tables
) {
    StockPriceDirectionTables* price_direction_tables =
        malloc(sizeof(StockPriceDirectionTables));
    if (!price_direction_tables) {
        fprintf(stderr, "malloc error");
        exit(EXIT_FAILURE);
    }

    const size_t table_count = stock_data_tables->table_count;
    price_direction_tables->tables =
        malloc(table_count * sizeof(StockPriceDirectionTable));
    if (!price_direction_tables->tables) {
        fprintf(stderr, "malloc error");
        exit(EXIT_FAILURE);
    }
    price_direction_tables->table_count = 0;
    price_direction_tables->capacity = table_count;

#pragma omp parallel for
    for (size_t i = 0; i < stock_data_tables->table_count; i++) {
        const StockDataTable* stock_data_table = &stock_data_tables->tables[i];
        StockPriceDirectionTable* price_direction_table =
            &price_direction_tables->tables[i];
        price_direction_table->stock_symbol = stock_data_table->stock_symbol;
        const size_t row_count = stock_data_table->row_count;
        price_direction_table->went_up =
            malloc(row_count * sizeof(bool));
        if (!price_direction_table->went_up) {
            fprintf(stderr, "malloc error");
            exit(EXIT_FAILURE);
        }
        price_direction_table->current_index = 0;
        price_direction_table->capacity = row_count;
        if (row_count > BUY_SELL_LAG) {
            for (size_t j = 0; j < row_count - BUY_SELL_LAG; j++) {
                const double current_high = stock_data_table->rows[j].high;
                const double future_low =
                    stock_data_table->rows[j + BUY_SELL_LAG].low;
                price_direction_table->went_up[j] =
                    future_low / current_high > 1.10;
                price_direction_table->current_index++;
            }
            price_direction_tables->table_count++;
        }
    }
    return price_direction_tables;
}

void print_stats_of_price_direction(
    const StockPriceDirectionTables* stock_price_direction_tables
) {
    for (size_t i = 0; i < stock_price_direction_tables->table_count; i++) {
        const StockPriceDirectionTable* price_direction_table =
            &stock_price_direction_tables->tables[i];
        char* stock_symbol = price_direction_table->stock_symbol;
        size_t number_up = 0;
        size_t total_number = 0;
        for (size_t j = 0; j < price_direction_table->current_index; j++) {
            if (price_direction_table->went_up[j]) {
                number_up++;
            }
            total_number++;
        }
        if (total_number > 0) {
            printf(
                "Symbol: %s, Percent_up: %f, Data Points: %lu\n",
                stock_symbol,
                (double)number_up / (double)total_number,
                price_direction_table->current_index
            );
        }
    }
}

void print_wait_times_of_price_direction(
    const StockDataTables* stock_data_tables
) {
    typedef struct {
        size_t days;
        size_t number_of_times;
    } TimesUp;
    typedef struct {
        TimesUp* times_up;
        size_t capacity;
    } StockTimesUp;
    const char* filename = "stock_stats.txt";

    char* path = NULL;
    if (asprintf(&path, "%s/%s", FINAL_DATA_FOLDER, filename) == -1) {
        fprintf(stderr, "asprintf failed");
        exit(EXIT_FAILURE);
    }
    FILE* file = fopen(path, "w");
    if (!file) {
        perror("fopen failed");
        free(path);
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < stock_data_tables->table_count; i++) {
        const StockDataTable* stock_data_table =
            &stock_data_tables->tables[i];
        char* stock_symbol = stock_data_table->stock_symbol;
        StockTimesUp stock_times_up = (StockTimesUp){
            .times_up = malloc(
                sizeof(TimesUp) * stock_data_table->row_count),
            .capacity = stock_data_table->row_count
        };
        if (!stock_times_up.times_up) {
            fprintf(stderr, "malloc failed");
            exit(EXIT_FAILURE);
        }
        for (size_t k = 0; k < stock_times_up.capacity; k++) {
            stock_times_up.times_up[k].days = k;
            stock_times_up.times_up[k].number_of_times = 0;
        }

        size_t last_true_index = -1;
        constexpr size_t buy_sell_lag = 5;
        if (stock_data_table->row_count < buy_sell_lag) {
            continue;
        }
        bool was_up = false;
        for (size_t j = 0;
             j < stock_data_table->row_count - buy_sell_lag;
             j++
        ) {
            if (stock_data_table->rows[j].high <
                stock_data_table->rows[j + buy_sell_lag].low &&
                !was_up
            ) {
                was_up = true;
                if (last_true_index != -1) {
                    const size_t wait = j - last_true_index;
                    stock_times_up.times_up[wait].number_of_times++;
                }
                last_true_index = j;
            } else if (stock_data_table->rows[j].high >=
                stock_data_table->rows[j + buy_sell_lag].low
            ) {
                was_up = false;
            }
        }

        size_t number_up = 0;
        size_t total_number = 0;
        for (size_t j = 0; j < stock_data_table->row_count; j++) {
            if (stock_data_table->rows[j].high <
                stock_data_table->rows[j + buy_sell_lag].low
            ) {
                number_up++;
            }
            total_number++;
        }

        const auto streaks = (StockTimesUp){
            .times_up = malloc(sizeof(TimesUp) * stock_data_table->row_count),
            .capacity = stock_data_table->row_count
        };
        if (!streaks.times_up) {
            fprintf(stderr, "malloc failed");
            exit(EXIT_FAILURE);
        }
        for (size_t k = 0; k < streaks.capacity; k++) {
            streaks.times_up[k].days = k;
            streaks.times_up[k].number_of_times = 0;
        }

        size_t streak = 0;
        for (size_t j = 0;
             j < stock_data_table->row_count - buy_sell_lag;
             j++
        ) {
            if ((streak == 0 && (stock_data_table->rows[j].high <
                stock_data_table->rows[j + buy_sell_lag].low)) ||

            ) {
                streak++;
            } else {
                if (streak > 0) {
                    streaks.times_up[streak].number_of_times++;
                }
                streak = 0;
            }
        }

        if (total_number > 0) {
            fprintf(
                file,
                "Symbol: %s, Percent_up: %f, Data Points: %lu\n",
                stock_symbol,
                (double)number_up / (double)total_number,
                stock_data_table->row_count
            );
        }

        for (size_t j = 0; j < stock_times_up.capacity; j++) {
            if (stock_times_up.times_up[j].number_of_times > 0) {
                fprintf(
                    file,
                    "  %zu days: %zu times\n",
                    stock_times_up.times_up[j].days,
                    stock_times_up.times_up[j].number_of_times
                );
            }
        }

        fprintf(file, "Up streaks:\n");
        for (size_t j = 0; j < streaks.capacity; j++) {
            if (streaks.times_up[j].number_of_times > 0) {
                fprintf(
                    file,
                    "  %zu days: %zu times\n",
                    streaks.times_up[j].days,
                    streaks.times_up[j].number_of_times
                );
            }
        }
        free(stock_times_up.times_up);
        free(streaks.times_up);
    }
    fclose(file);
    free(path);
}


void load_data_and_extract_price_diffs() {
    constexpr uint16_t start_year = START_YEAR;
    constexpr uint16_t end_year = END_YEAR;
    constexpr uint16_t mid_year = 2020;

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

    bool success = load_stock_data_from_disk(
        past_stock_data_tables,
        &start_year,
        &mid_year,
        INTERMEDIATE_DATA_FOLDER
    );
    if (!success) {
        exit(1);
    }
    print_wait_times_of_price_direction(past_stock_data_tables);


    const StockPriceDirectionTables* stock_price_direction_tables =
        get_price_direction_tables(past_stock_data_tables);
    // print_stats_of_price_direction(stock_price_direction_tables);

    // // future_stock_data_tables needs to be freed
    // // -------------------------------------------------------------------------
    // StockDataTables* future_stock_data_tables =
    //     malloc(sizeof(StockDataTables));
    // if (future_stock_data_tables == NULL) {
    //     perror("malloc failed");
    //     exit(EXIT_FAILURE);
    // }
    // memset(future_stock_data_tables, 0, sizeof(StockDataTables));
    // future_stock_data_tables->tables = nullptr;
    // future_stock_data_tables->table_count = 0;
    // future_stock_data_tables->capacity = 0;
    //
    // success = load_stock_data_from_disk(
    //     future_stock_data_tables,
    //     &mid_year,
    //     &end_year,
    //     INTERMEDIATE_DATA_FOLDER
    // );
    // if (!success) {
    //     exit(1);
    // }
    //
    // // past_stock_data_tables freed
    // // -------------------------------------------------------------------------
    // free_stock_data_tables(past_stock_data_tables);
    //
    // // future_stock_data_tables freed
    // // -------------------------------------------------------------------------
    // free_stock_data_tables(future_stock_data_tables);
}
