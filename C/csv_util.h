#ifndef CSV_UTIL_H
#define CSV_UTIL_H

#include <stddef.h>
#include <sys/types.h>

typedef struct {
    u_int16_t year;
    u_int8_t month;
    u_int8_t day;
} Date;

typedef struct {
    Date date;
    double open;
    double high;
    double low;
    double close;
    double volume;
} StockDataRow;

typedef struct {
    char* stock_symbol;
    StockDataRow* rows;
    size_t row_count;
} StockDataTable;

void read_stock_csv(
    const char* filename,
    StockDataTable* table,
    const u_int16_t* start_year,
    const u_int16_t* end_year
);

#endif //CSV_UTIL_H
