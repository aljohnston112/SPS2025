#ifndef CSVREADER_H
#define CSVREADER_H

#include "CsvCursor.h"
#include "MappedFileCursor.h"
#include "StringSpanner.h"

typedef struct {
    const char* endPtr;
    char* ptr;
    char delimiter;
    MappedFileCursor* stream;
    StringSpanner unquoted_cell_spanner;
    CsvCursor cursor;
} CsvReader;

void csv_reader_init(
    CsvReader* reader,
    MappedFileCursor* file_cursor,
    char delimiter
);
int csv_reader_try_parse(CsvReader* reader);
int csv_reader_read_row(CsvReader* reader);

#endif //CSVREADER_H
