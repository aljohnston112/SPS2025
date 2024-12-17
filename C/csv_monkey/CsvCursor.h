#ifndef CSVCURSOR_H
#define CSVCURSOR_H

#include "CsvCell.h"

typedef struct {
    size_t count;
    CsvCell* cells[32];
} CsvCursor;

const CsvCell* csv_cursor_with_column_name(const CsvCursor* cursor, const char* value);


#endif //CSVCURSOR_H
