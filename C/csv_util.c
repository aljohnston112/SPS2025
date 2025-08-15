#include "csv_util.h"

#include <assert.h>
#include <stdio.h>

#include "config.h"
#include "../CPP/csv_lion/CsvReader.h"
#include "../CPP/csv_lion/MappedFileCursor.h"

/**
 * Reads stock data from disk.
 *
 * @param filename The name of the stock data csv file.
 * @param table The table to be filled with the stock data from the file.
 *              The table must be allocated by the caller before calling this method.
 *              It is up to the caller to ensure the table is large enough
 *              to hold the stock data.
* @param start_year The first year, inclusive,
 *                   of the stock data to be loaded.
 *                   If null, stock data will be loaded from the beginning.
 * @param end_year The last year, exclusive,
 *                 of the stock data to be loaded.
 *                 If null, stock data will be loaded to the end.
 */
void read_stock_csv(
    const char* filename,
    StockDataTable* table,
    const u_int16_t* start_year,
    const u_int16_t* end_year
) {
    MappedFileCursor file_cursor;
    if (mapped_file_cursor_map_file(&file_cursor, filename) != 0) {
        fprintf(stderr, "Failed to load stock data from file: %s", filename);
        table->row_count = 0;
        table->capacity = 0;
        return;
    }

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

    // Find the first row with the given start year
    // or the first data point if it is greater than the start year
    size_t data_index = 0;
    if (start_year != NULL) {
        while (csv_reader_read_row(&reader)) {
            const char* date = dateCell->ptr;
            u_int16_t year;
            extract_uint16_t(date, date + 4, &year);

            if (end_year != NULL && year > *end_year) {
                data_index--;
                break;
            }
            if (year >= *start_year) {
                StockDataRow* current_row = &table->rows[0];

                // Extract the date
                u_int8_t current_month;
                extract_uint8_t(date + 4, date + 6, &current_month);
                u_int8_t current_day;
                extract_uint8_t(date + 6, date + 8, &current_day);
                current_row->date = (Date){
                    .year = year,
                    .month = current_month,
                    .day = current_day
                };

                // Load the first row
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

    // Load the data up to right before the end year
    while (csv_reader_read_row(&reader)) {
        const char* date = dateCell->ptr;

        StockDataRow* current_row = &table->rows[data_index];
        u_int16_t year;
        extract_uint16_t(date, date + 4, &year);

        if (end_year != NULL && year >= *end_year) {
            break;
        }

        // Extract the date
        u_int8_t current_month;
        extract_uint8_t(date + 4, date + 6, &current_month);
        u_int8_t current_day;
        extract_uint8_t(date + 6, date + 8, &current_day);
        current_row->date = (Date){
            .year = year,
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

    if (data_index > table->capacity) {
        fprintf(
            stderr,
            "Not enough space in the stock data tables; "
            "need %lu more rows\n",
            data_index - table->capacity - 1
        );
    }
    mapped_file_cursor_clean_up(&file_cursor);
    table->row_count = data_index;
}

/**
 * Fetches the min and max date of all stock data table rows.
 *
 * @param stock_data_tables The stock data tables.
 * @param table_count The number of stock data tables.
 * @param min_date Will contain the min date of all stock table rows on return.
 * @param max_date Will contain the max date of all stock table rows on return.
 */
void get_min_and_max_dates(
    StockDataTable** stock_data_tables,
    const size_t table_count,
    Date* min_date,
    Date* max_date
) {
    assert(stock_data_tables != nullptr);
    assert(table_count > 0);

    // init with first
    const StockDataTable* first_table = stock_data_tables[0];
    assert(first_table != nullptr);
    assert(first_table->row_count > 0);

    const StockDataRow* first_row_in_first_table = &first_table->rows[0];
    min_date->year = first_row_in_first_table->date.year;
    min_date->month = first_row_in_first_table->date.month;
    min_date->day = first_row_in_first_table->date.day;
    const StockDataRow* last_row_in_first_table =
        &first_table->rows[first_table->row_count - 1];
    max_date->year = last_row_in_first_table->date.year;
    max_date->month = last_row_in_first_table->date.month;
    max_date->day = last_row_in_first_table->date.day;

    // Find the min and max dates
    for (size_t i = 0; i < table_count; i++) {
        const StockDataTable* table = stock_data_tables[i];
        assert(table != nullptr);
        assert(table->row_count > 0);

        // Check the first row for the min date
        const StockDataRow* first_row = &table->rows[0];
        const Date* first_date = &first_row->date;

        if (is_date_less(first_date, min_date)) {
            min_date->year = first_date->year;
            min_date->month = first_date->month;
            min_date->day = first_date->day;
        }

        // Check the last row for the max date
        const StockDataRow* last_row = &table->rows[table->row_count - 1];
        const Date* last_date = &last_row->date;
        if (is_date_less(max_date, last_date)) {
            max_date->year = last_date->year;
            max_date->month = last_date->month;
            max_date->day = last_date->day;
        }
    }
}
