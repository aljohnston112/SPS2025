#ifndef CSVCELL_H
#define CSVCELL_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CsvCell {
    char* ptr;
    size_t size;
} CsvCell;

char* csv_cell_as_str(const CsvCell* self);
int csv_cell_as_int(const CsvCell* self, int* out_value);
int csv_cell_as_double(const CsvCell* self, double* out_value);

#ifdef __cplusplus
}
#endif

#endif // CSVCELL_H
