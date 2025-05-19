#ifndef CSV_UTIL_H
#define CSV_UTIL_H

#include <stddef.h>
#include <sys/types.h>

typedef struct {
    u_int16_t year;
    u_int8_t month;
    u_int8_t day;
    double open;
    double high;
    double low;
    double close;
    double volume;
} Row;

typedef struct RowArray {
    char* stock_symbol;
    Row* rows;
    size_t data_size;
} RowArray;

void read_stock_csv(const char* filename, RowArray* rows);

void read_stock_csv_to_year(
    const char* filename,
    RowArray* rows,
    u_int16_t end_year
);

void read_stock_csv_from_year(
    const char* filename,
    RowArray* rows,
    u_int16_t start_year
);

#endif //CSV_UTIL_H
