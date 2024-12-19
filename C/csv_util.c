#include "csv_util.h"

#include <stdlib.h>

#include "csv_lion/CsvReader.h"
#include "csv_lion/MappedFileCursor.h"

void read_stock_csv(const char* filename, size_t* data_size, Row* data) {
    MappedFileCursor file_cursor;
    mapped_file_cursor_map_file(&file_cursor, filename);

    CsvReader reader;
    csv_reader_init(&reader, &file_cursor, ',');
    csv_reader_read_row(&reader);
    const CsvCursor* row = csv_reader_row(&reader);
    const CsvCell* dateCell = csv_cursor_with_column_name(row, "<DATE>");
    const CsvCell* openCell = csv_cursor_with_column_name(row, "<OPEN>");
    const CsvCell* highCell = csv_cursor_with_column_name(row, "<HIGH>");
    const CsvCell* lowCell = csv_cursor_with_column_name(row, "<LOW>");
    const CsvCell* closeCell = csv_cursor_with_column_name(row, "<CLOSE>");
    const CsvCell* volumeCell = csv_cursor_with_column_name(row, "<VOL>");

    *data_size = 15844;
    size_t data_index = 0;
    while (csv_reader_read_row(&reader)) {
        const char* date = dateCell->ptr;

        extract_uint8_t(date + 4, date + 6, &data[data_index].month);
        extract_uint8_t(date + 6, date + 8, &data[data_index].day);

        csv_cell_as_double(openCell, &data[data_index].open);
        csv_cell_as_double(highCell, &data[data_index].high);
        csv_cell_as_double(lowCell, &data[data_index].low);
        csv_cell_as_double(closeCell, &data[data_index].close);
        csv_cell_as_double(volumeCell, &data[data_index].volume);
        data_index++;
    }

    mapped_file_cursor_clean_up(&file_cursor);

    *data_size = data_index;
   // printData(data, data_index);
}
