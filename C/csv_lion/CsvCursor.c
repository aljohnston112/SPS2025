#include "CsvCursor.h"

#include <stdlib.h>
#include <string.h>

/**
 * @param cursor A cursor pointing to the header row's start in a csv file.
 * @param value Header text to search for.
 * @return A csv cell with text matching value.
 */
const CsvCell* csv_cursor_with_column_name(const CsvCursor* cursor, const char* value) {
    for (size_t i = 0; i < cursor->count; i++) {
        char* csv_column_title = csv_cell_as_str(&cursor->cells[i]);
        if (strcmp(value, csv_column_title) == 0) {
            free(csv_column_title);
            return &cursor->cells[i];
        }
        free(csv_column_title);
    }
    return nullptr;
}
