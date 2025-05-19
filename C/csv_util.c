#include "csv_util.h"

#include "csv_lion/CsvReader.h"
#include "csv_lion/MappedFileCursor.h"

void read_stock_csv(const char* filename, RowArray* rows) {
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

    size_t data_index = 0;
    while (csv_reader_read_row(&reader)) {
        const char* date = dateCell->ptr;

        Row* current_row = &rows->rows[data_index];
        extract_uint16_t(date, date + 4, &current_row->year);
        extract_uint8_t(date + 4, date + 6, &current_row->month);
        extract_uint8_t(date + 6, date + 8, &current_row->day);

        csv_cell_as_double(openCell, &current_row->open);
        csv_cell_as_double(highCell, &current_row->high);
        csv_cell_as_double(lowCell, &current_row->low);
        csv_cell_as_double(closeCell, &current_row->close);
        csv_cell_as_double(volumeCell, &current_row->volume);
        data_index++;
    }

    mapped_file_cursor_clean_up(&file_cursor);
    rows->data_size = data_index;
}

/**
 * Reads data up to the end year.
 *
 * @param filename
 * @param rows
 * @param end_year Set to 0 to load all data.
 */
void read_stock_csv_to_year(const char* filename, RowArray* rows, u_int16_t end_year) {
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

    size_t data_index = 0;
    while (csv_reader_read_row(&reader)) {
        const char* date = dateCell->ptr;

        Row* current_row = &rows->rows[data_index];
        u_int16_t year;
        extract_uint16_t(date, date + 4, &year);
        if (year >= end_year) {
            break;
        }

        current_row->year = year;
        extract_uint8_t(date + 4, date + 6, &current_row->month);
        extract_uint8_t(date + 6, date + 8, &current_row->day);

        csv_cell_as_double(openCell, &current_row->open);
        csv_cell_as_double(highCell, &current_row->high);
        csv_cell_as_double(lowCell, &current_row->low);
        csv_cell_as_double(closeCell, &current_row->close);
        csv_cell_as_double(volumeCell, &current_row->volume);
        data_index++;
    }

    mapped_file_cursor_clean_up(&file_cursor);
    rows->data_size = data_index;
}

/**
 * Reads data from the year given to the end of the data stream.
 *
 * @param filename
 * @param rows
 * @param start_year Set to 0 to load all data.
 */
void read_stock_csv_from_year(const char* filename, RowArray* rows, u_int16_t start_year) {
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

    size_t data_index = 0;
    while (csv_reader_read_row(&reader)) {
        const char* date = dateCell->ptr;
        u_int16_t year;
        extract_uint16_t(date, date + 4, &year);
        if (year >= start_year) {
            break;
        }
    }

    while (csv_reader_read_row(&reader)) {
        const char* date = dateCell->ptr;

        Row* current_row = &rows->rows[data_index];
        extract_uint16_t(date, date + 4, &current_row->year);
        extract_uint8_t(date + 4, date + 6, &current_row->month);
        extract_uint8_t(date + 6, date + 8, &current_row->day);

        csv_cell_as_double(openCell, &current_row->open);
        csv_cell_as_double(highCell, &current_row->high);
        csv_cell_as_double(lowCell, &current_row->low);
        csv_cell_as_double(closeCell, &current_row->close);
        csv_cell_as_double(volumeCell, &current_row->volume);
        data_index++;
    }

    mapped_file_cursor_clean_up(&file_cursor);
    rows->data_size = data_index;
}
