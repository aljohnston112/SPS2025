#ifndef CSV_UTIL_H
#define CSV_UTIL_H

#include <stddef.h>

typedef union
{
    double* doubles;
    char* ints;
} TwoDimensionalArrayElement;

void read_stock_csv(const char* filename, size_t* data_size, TwoDimensionalArrayElement** data);
#endif //CSV_UTIL_H
