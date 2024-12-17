#ifndef WAVELET_CSVCELL_H
#define WAVELET_CSVCELL_H

#include <stdlib.h>

typedef struct CsvCell {
    const char* ptr;
    size_t size;

    char* (*as_str)(const struct CsvCell* self);
    int (*as_int)(const struct CsvCell* self, int* out_value);
    double (*as_double)(const struct CsvCell* self, double* out_value);
} CsvCell;

char* CsvCell_as_str(const CsvCell* self);
int CsvCell_as_int(const CsvCell* self, int* out_value);
int CsvCell_as_double(const CsvCell* self, double* out_value);
CsvCell create_csv_cell(const char* ptr, size_t size);

#endif // WAVELET_CSVCELL_H
