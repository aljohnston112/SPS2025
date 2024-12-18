#ifndef CSV_UTIL_H
#define CSV_UTIL_H

#include <stddef.h>

typedef struct
{
    char month;
    char day;
    double open;
    double close;
    double high;
    double low;
    double volume;

} Row;

void read_stock_csv(const char* filename, size_t* data_size, Row* data);
void printData(const Row* data, size_t data_size);
#endif //CSV_UTIL_H
