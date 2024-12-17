#include "CsvCursor.h"

#include <string.h>

/**
 * @param cursor A cursor pointing to the header row's start in a csv file.
 * @param value Header text to search for.
 * @return A csv cell with text matching value.
 */
const CsvCell* csv_cursor_with_column_name(const CsvCursor* cursor, const char* value) {
    for (size_t i = 0; i < cursor->count; i++) {
        if (strcmp(value, csv_cell_as_str(cursor->cells[i])) == 0) {
            return cursor->cells[i];
        }
    }
    return nullptr;
}
