#ifndef CSV_UTIL_H
#define CSV_UTIL_H

#include <stddef.h>
#include <sys/types.h>

typedef struct
{
    u_int8_t month;
    u_int8_t day;
    double open;
    double high;
    double low;
    double close;
    double volume;

} Row;

void read_stock_csv(const char* filename, size_t* data_size, Row* data);
void printData(const Row* data, size_t data_size);
#endif //CSV_UTIL_H
