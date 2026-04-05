#include "CsvReader.h"

#include <stdio.h>
#include <string.h>

#include "MappedFileCursor.h"

#include "CsvCursor.h"
#include "../../C/config.h"

void csv_reader_init(
    CsvReader* reader,
    MappedFileCursor* file_cursor,
    const char delimiter
) {
    reader->ptr = mapped_file_cursor_buf(file_cursor);
    reader->endPtr = reader->ptr + mapped_file_cursor_size(file_cursor);
    reader->delimiter = delimiter;
    reader->stream = file_cursor;
    string_spanner_init(
        &reader->unquoted_cell_spanner,
        delimiter,
        '\r',
        '\n',
        0
    );
}

int csv_reader_try_parse_old(CsvReader* reader) {
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
            if (reader->cursor.count >= NUMBER_OF_COLUMNS_IN_CSV) {
                fprintf(
                    stderr,
                    "MAX_NUMBER_OF_COLUMNS_IN_CSV was not high enough\n"
                );
                return -1;
            }
            reader->ptr = currentCharacterPtr + 1;
            return 1;
        }

        char* cell_start = currentCharacterPtr;

        while (true) {
            const int rc = string_spanner_operator(
                &reader->unquoted_cell_spanner,
                currentCharacterPtr
            );
            if (rc != 16) {
                currentCharacterPtr += rc;
                break;
            }
            currentCharacterPtr += 16;
            if (currentCharacterPtr >= reader->endPtr) {
                fprintf(stderr, "File did not end with a newline");
                return -1;
            }
        }

        cell->ptr = cell_start;
        cell->size = (uint64_t)(currentCharacterPtr - cell_start);
        ++reader->cursor.count;
        if (reader->cursor.count >= NUMBER_OF_COLUMNS_IN_CSV) {
            fprintf(
                stderr,
                "MAX_NUMBER_OF_COLUMNS_IN_CSV was not high enough\n"
            );
            return -1;
        }

        if (*currentCharacterPtr == reader->delimiter) {
            ++cell;
            ++currentCharacterPtr;
        } else {
            reader->ptr = currentCharacterPtr + 1;
            return 1;
        }
    }
    return 0;
}

int csv_reader_try_parse(CsvReader* reader) {
    if (reader->ptr >= reader->endPtr) {
        return 0;
    }

    CsvCell* cell = &reader->cursor.cells[0];
    reader->cursor.count = 0;
    char* start_ptr = reader->ptr;

    const __m512i comma = _mm512_set1_epi8(',');
    __m512i chunk = _mm512_loadu_si512((__m512i*)start_ptr);
    register __mmask64 mask = _mm512_cmpeq_epu8_mask(chunk, comma);

    int64_t number_of_commas = _mm_popcnt_u64(mask);
    number_of_commas = number_of_commas < NUMBER_OF_COLUMNS_IN_CSV
                           ? number_of_commas
                           : NUMBER_OF_COLUMNS_IN_CSV - 1;

    cell->ptr = start_ptr;
    cell->size = _tzcnt_u64(mask);
    uint64_t last_index = cell->size;
    mask = _blsr_u64(mask);
    cell = &reader->cursor.cells[++reader->cursor.count];
    for (int64_t i = 1; i < number_of_commas; i++) {
        cell->ptr = start_ptr + last_index + 1;
        const uint64_t index = _tzcnt_u64(mask);
        cell->size = index - last_index - 1;
        mask = _blsr_u64(mask);
        cell = &reader->cursor.cells[++reader->cursor.count];
        last_index = index;
    }

    const uint64_t last_index_of_first = last_index;
    bool overrun = false;
    if (reader->cursor.count < NUMBER_OF_COLUMNS_IN_CSV - 1) {
        overrun = true;
        chunk = _mm512_loadu_si512((__m512i*)(start_ptr + 64));
        mask = _mm512_cmpeq_epu8_mask(chunk, comma);

        const int64_t commas_left =
            NUMBER_OF_COLUMNS_IN_CSV - number_of_commas - 1;
        for (int64_t i = 0; i < commas_left; i++) {
            const uint64_t index = _tzcnt_u64(mask);
            cell->ptr = i == 0
                            ? start_ptr + last_index + 1
                            : start_ptr + last_index + 65;
            cell->size = i == 0
                             ? (64 - last_index) + index - 1
                             : index - last_index - 1;
            cell = &reader->cursor.cells[++reader->cursor.count];
            last_index = index;
        }
        last_index += 64;
    }

    const __m512i new_line = _mm512_set1_epi8('\n');
    chunk = _mm512_loadu_si512((__m512i*)start_ptr);
    mask = _mm512_cmpeq_epu8_mask(chunk, new_line);
    uint64_t index = _tzcnt_u64(mask);
    if (mask == 0) {
        chunk = _mm512_loadu_si512((__m512i*)(start_ptr + 64));
        mask = _mm512_cmpeq_epu8_mask(chunk, new_line);
        index = _tzcnt_u64(mask) + 64;
    }
    if (mask == 0) {
        return -1;
    }

    cell->ptr = start_ptr + (overrun ? last_index : last_index_of_first) + 1;
    cell->size = index - last_index - 2;
    ++reader->cursor.count;
    reader->ptr = cell->ptr + cell->size + 2;
    return 1;
}

int csv_reader_read_row(CsvReader* reader) {
    const int result = csv_reader_try_parse(reader);
    return result;
}
