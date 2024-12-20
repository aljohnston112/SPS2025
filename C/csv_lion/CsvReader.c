#include "CsvReader.h"

#include "MappedFileCursor.h"

#ifndef WAVELET_csv_reader_H
#define WAVELET_csv_reader_H

#include "CsvCursor.h"

void csv_reader_init(CsvReader* reader, MappedFileCursor* file_cursor, const char delimiter) {
    reader->ptr = mapped_file_cursor_buf(file_cursor);
    reader->endPtr = reader->ptr + mapped_file_cursor_size(file_cursor);
    reader->delimiter = delimiter;
    reader->stream = file_cursor;
    reader->cursor;
    string_spanner_init(&reader->unquoted_cell_spanner, delimiter, '\r', '\n', 0);
}

int csv_reader_try_parse(CsvReader* reader) {
    char* currentCharacterPtr = reader->ptr;
    CsvCell* cell = &reader->cursor.cells[0];
    reader->cursor.count = 0;

    while (*currentCharacterPtr == '\r' || *currentCharacterPtr == '\n') {
        ++currentCharacterPtr;
    }

    while (currentCharacterPtr < reader->endPtr) {
        if (*currentCharacterPtr == '\r' || *currentCharacterPtr == '\n') {
            cell->ptr = nullptr;
            cell->size = 0;

            reader->cursor.count++;
            reader->ptr = currentCharacterPtr + 1;
            return true;
        }

        char* cell_start = currentCharacterPtr;

        // TODO This should be a loop
        // It is a hard limit of 32 characters
        const size_t rc = string_spanner_operator(
            &reader->unquoted_cell_spanner,
            currentCharacterPtr
            );
        if (rc != 16) {
            currentCharacterPtr += rc;
        } else {
            const size_t rc2 = string_spanner_operator(
                &reader->unquoted_cell_spanner,
                currentCharacterPtr + 16
            );
            currentCharacterPtr += rc2 + 16;
        }

        cell->ptr = cell_start;
        cell->size = currentCharacterPtr - cell_start;
        ++reader->cursor.count;

        if (*currentCharacterPtr == reader->delimiter) {
            ++cell;
            ++currentCharacterPtr;
        } else {
            reader->ptr = currentCharacterPtr + 1;
            return true;
        }
    }
    return false;
}

bool csv_reader_read_row(CsvReader* reader) {
    int canContinue = false;
    const char* startP = reader->ptr;

    if (csv_reader_try_parse(reader)) {
        mapped_file_cursor_consume(reader->stream, reader->ptr - startP);
        canContinue = true;
    }
    return canContinue;
}

CsvCursor* csv_reader_row(CsvReader* reader) {
    return &reader->cursor;
}

#endif //WAVELET_csv_reader_H
