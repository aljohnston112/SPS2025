#ifndef CSV_UTIL_H
#define CSV_UTIL_H

#include <sys/types.h>

#include "date_util.h"

typedef struct {
    Date date;
    double open;
    double high;
    double low;
    double close;
    double volume;
} StockDataRow;

typedef struct {
    const char* stock_symbol;
    StockDataRow* rows;
    size_t row_count;
    size_t capacity;
} StockDataTable;

bool read_stock_csv(
    const char* filename,
    StockDataTable* table,
    const u_int16_t* start_year,
    const u_int16_t* end_year
);

void get_min_and_max_dates(
    StockDataTable** stock_data_tables,
    size_t table_count,
    Date* min_date,
    Date* max_date
);

#endif //CSV_UTIL_H
