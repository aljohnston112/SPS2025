#ifndef CSVCELL_H
#define CSVCELL_H

#include <sys/types.h>

#ifdef __cplusplus
#include <cstddef>

extern "C" {
#else
#include <stddef.h>
#endif

typedef struct CsvCell {
    char* ptr;
    long int size;
} CsvCell;

char* csv_cell_as_str(const CsvCell* self);
int csv_cell_as_double(const CsvCell* self, double* out_value);
int extract_uint8_t(const char* first, const char* last, u_int8_t* out_value);
int extract_uint16_t(const char* first, const char* last, u_int16_t* out_value);
int extract_uint64_t(const char* first, const char* last, u_int64_t* out_value);

int extract_long(const char* first, const char* last, long* out_value);

#ifdef __cplusplus
}
#endif

#endif // CSVCELL_H
