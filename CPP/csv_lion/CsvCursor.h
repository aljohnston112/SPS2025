#ifndef CSVCURSOR_H
#define CSVCURSOR_H

#include "CsvCell.h"
#include "../../C/config.h"

typedef struct {
    size_t count;
    CsvCell cells[NUMBER_OF_COLUMNS_IN_CSV];
} CsvCursor;

const CsvCell* get_cell_with_column_name(const CsvCursor* cursor, const char* column_name);


#endif //CSVCURSOR_H
