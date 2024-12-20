#include "CsvCursor.h"

#include <stdlib.h>
#include <string.h>

/**
 * @param self A cursor pointing to the header row's start in a csv file.
 * @param value Header text to search for.
 * @return A csv cell with text matching value.
 */
const CsvCell* csv_cursor_with_column_name(const CsvCursor* self, const char* value) {
    for (size_t i = 0; i < self->count; i++) {
        char* csv_column_title = csv_cell_as_str(&self->cells[i]);
        if (strcmp(value, csv_column_title) == 0) {
            free(csv_column_title);
            return &self->cells[i];
        }
        free(csv_column_title);
    }
    return nullptr;
}
