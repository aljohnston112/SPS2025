#include "csv_util.h"

#include <stdio.h>
#include <stdlib.h>

#include "csv_monkey/CsvReader.h"
#include "csv_monkey/MappedFileCursor.h"

void printData(const TwoDimensionalArrayElement* data, const size_t data_size) {
    for (size_t i = 0; i < 7; i++) {
        printf("Row %zu:\n", i);
        const TwoDimensionalArrayElement element = data[i];
        if (i < 2) {
            printf("  ints: ");
            for (size_t j = 0; j < data_size; j++) {
                printf("%d ", element.ints[j]);
            }
            printf("\n");
        } else {
            printf("  doubles: ");
            for (size_t j = 0; j < data_size; j++) {
                printf("%.2f ", element.doubles[j]);
            }
            printf("\n");
        }
    }
}

void read_stock_csv(const char* filename, size_t* data_size, const TwoDimensionalArrayElement* data) {
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

    *data_size = 15844;
    size_t data_index = 0;
    while (csv_reader_read_row(&reader)) {
        const char* date = dateCell->ptr;
        char buffer[3];
        buffer[0] = date[4];
        buffer[1] = date[5];
        buffer[2] = '\0';
        const long month = strtol(buffer, nullptr, 10);
        data[0].ints[data_index] = (char)month;

        buffer[0] = date[6];
        buffer[1] = date[7];
        buffer[2] = '\0';
        const long day = strtol(buffer, nullptr, 10);
        data[1].ints[data_index] = (char)day;

        csv_cell_as_double(openCell, &data[2].doubles[data_index]);
        csv_cell_as_double(highCell, &data[3].doubles[data_index]);
        csv_cell_as_double(lowCell, &data[4].doubles[data_index]);
        csv_cell_as_double(closeCell, &data[5].doubles[data_index]);
        csv_cell_as_double(volumeCell, &data[6].doubles[data_index]);
        data_index++;
    }

    mapped_file_cursor_clean_up(&file);

    *data_size = data_index;
   // printData(data, data_index);
}
