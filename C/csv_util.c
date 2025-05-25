#include "csv_util.h"

#include "../CPP/csv_lion/CsvReader.h"
#include "../CPP/csv_lion/MappedFileCursor.h"

/**
 * Reads stock data from the given start year up to the given end year.
 * The given end year is not included in the data.
 *
 * @param filename
 * @param rows
 * @param start_year
 * @param end_year
 */
void read_stock_csv(
    const char* filename,
    RowArray* rows,
    const u_int16_t* start_year,
    const u_int16_t* end_year
) {
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
    if (start_year != NULL) {
        while (csv_reader_read_row(&reader)) {
            const char* date = dateCell->ptr;
            u_int16_t year;
            extract_uint16_t(date, date + 4, &year);
            if (year >= *start_year) {
                Row* current_row = &rows->rows[0];

                // Extract the date
                u_int16_t current_year = year;
                extract_uint16_t(date, date + 4, &current_year);
                u_int8_t current_month;
                extract_uint8_t(date + 4, date + 6, &current_month);
                u_int8_t current_day;
                extract_uint8_t(date + 6, date + 8, &current_day);
                current_row->date = (Date){
                    .year = current_year,
                    .month = current_month,
                    .day = current_day
                };

                csv_cell_as_double(openCell, &current_row->open);
                csv_cell_as_double(highCell, &current_row->high);
                csv_cell_as_double(lowCell, &current_row->low);
                csv_cell_as_double(closeCell, &current_row->close);
                csv_cell_as_double(volumeCell, &current_row->volume);
                break;
            }
        }
        data_index++;
    }

    while (csv_reader_read_row(&reader)) {
        const char* date = dateCell->ptr;

        Row* current_row = &rows->rows[data_index];
        u_int16_t year;
        extract_uint16_t(date, date + 4, &year);

        if (end_year != NULL && year >= *end_year) {
            break;
        }

        // Extract the date
        u_int16_t current_year = year;
        extract_uint16_t(date, date + 4, &current_year);
        u_int8_t current_month;
        extract_uint8_t(date + 4, date + 6, &current_month);
        u_int8_t current_day;
        extract_uint8_t(date + 6, date + 8, &current_day);
        current_row->date = (Date){
            .year = current_year,
            .month = current_month,
            .day = current_day
        };

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
