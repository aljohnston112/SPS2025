#include "csv_util.h"

#include <stddef.h>
#include <stdlib.h>

#include "csv_monkey/CsvReader.h"
#include "csv_monkey/MappedFileCursor.h"

double** read_stock_csv(const char* filename, size_t* data_size) {
    MappedFileCursor file;
    mapped_file_cursor_map_file(&file, filename);

    CsvReader reader;
    csv_reader_init(&reader, &file, ',');
    csv_reader_read_row(&reader);
    const CsvCursor* row = csv_reader_row(&reader);
    const CsvCell* dateCell = csv_cursor_with_column_name(row, "<DATE>");
    const CsvCell* openCell = csv_cursor_with_column_name(row, "<OPEN>");
    const CsvCell* highCell = csv_cursor_with_column_name(row, "<HIGH>");
    const CsvCell* lowCell = csv_cursor_with_column_name(row, "<LOW>");
    const CsvCell* closeCell = csv_cursor_with_column_name(row, "<CLOSE>");
    const CsvCell* volumeCell = csv_cursor_with_column_name(row, "<VOL>");

    double** data = malloc(7 * sizeof(double*));
    for (int i = 0; i < 7; i++) {
        data[i] = (double*)malloc(*data_size * sizeof(double));
    }
    size_t data_index = 0;
    while (csv_reader_read_row(&reader)) {
        char* date = dateCell->ptr;
        char* monthEnd = date + 6;
        const long month = strtol(date + 4, &monthEnd, 10);
        data[0][data_index] = month;

        char* dayEnd = date + 8;
        const long day = strtol(date + 6, &dayEnd, 10);
        data[1][data_index] = day;

        csv_cell_as_double(openCell, &data[0][data_index]);
        csv_cell_as_double(highCell, &data[1][data_index]);
        csv_cell_as_double(lowCell, &data[2][data_index]);
        csv_cell_as_double(closeCell, &data[3][data_index]);
        csv_cell_as_double(volumeCell, &data[4][data_index]);
        data_index++;
    }

    mapped_file_cursor_clean_up(&file);

    *data_size = data_index;
    return data;
}