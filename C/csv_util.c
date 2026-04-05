#include "csv_util.h"

#include <assert.h>
#include <stdio.h>

#include "config.h"
#include "../CPP/csv_lion/CsvReader.h"
#include "../CPP/csv_lion/MappedFileCursor.h"

typedef struct {
    const CsvCell* date;
    const CsvCell* open;
    const CsvCell* high;
    const CsvCell* low;
    const CsvCell* close;
    const CsvCell* volume;
} StockTableCsvHeaderCells;

static inline bool parse_row(
    const StockTableCsvHeaderCells header_cells,
    StockDataRow* current_row,
    const u_int16_t year
) {
    const char* date = header_cells.date->ptr;
    const uint8_t current_month =
        (uint8_t)((date[4] - '0') * 10 + (date[5] - '0'));
    const uint8_t current_day =
        (uint8_t)((date[6] - '0') * 10 + (date[7] - '0'));
    current_row->date = (Date){
        .year = year,
        .month = current_month,
        .day = current_day
    };

    register int row_status = 0;
    row_status |= csv_cell_as_double(header_cells.open, &current_row->open);
    row_status |= csv_cell_as_double(header_cells.high, &current_row->high);
    row_status |= csv_cell_as_double(header_cells.low, &current_row->low);
    row_status |= csv_cell_as_double(header_cells.close, &current_row->close);
    row_status |= csv_cell_as_double(header_cells.volume, &current_row->volume);
    if (unlikely(row_status != 0)) {
        return false;
    }
    return true;
}

/**
 * Reads stock data from disk.
 *
 * @param filename The name of the stock data csv file.
 * @param table The table to be filled with the stock data from the file.
 *              The rows of the table must be allocated by the caller
 *              before calling this method.
 *              It is up to the caller to ensure the table is large enough
 *              to hold the stock data.
* @param start_year The first year, inclusive,
 *                   of the stock data to be loaded.
 *                   If null, stock data will be loaded from the beginning.
 * @param end_year The last year, exclusive,
 *                 of the stock data to be loaded.
 *                 If null, stock data will be loaded to the end.
 */
bool read_stock_csv(
    const char* filename,
    StockDataTable* table,
    const u_int16_t* start_year,
    const u_int16_t* end_year
) {
    MappedFileCursor file_cursor;
    if (mapped_file_cursor_map_file(&file_cursor, filename) != 0) {
        if (mapped_file_cursor_map_file(&file_cursor, filename) == -1) {
            table->row_count = 0;
            table->capacity = 0;
            goto error;
        }
        mapped_file_cursor_clean_up(&file_cursor);
        return true;
    }

    CsvReader reader;
    csv_reader_init(&reader, &file_cursor, ',');

    // Parse header
    if (csv_reader_read_row(&reader) == -1) {
        goto error;
    }
    const CsvCursor* row_cursor = &reader.cursor;
    const CsvCell* dateCell =
        get_cell_with_column_name(row_cursor, "<DATE>");
    if (dateCell == nullptr) {
        goto error;
    }
    const CsvCell* openCell =
        get_cell_with_column_name(row_cursor, "<OPEN>");
    if (openCell == nullptr) {
        goto error;
    }
    const CsvCell* highCell =
        get_cell_with_column_name(row_cursor, "<HIGH>");
    if (highCell == nullptr) {
        goto error;
    }
    const CsvCell* lowCell =
        get_cell_with_column_name(row_cursor, "<LOW>");
    if (lowCell == nullptr) {
        goto error;
    }
    const CsvCell* closeCell =
        get_cell_with_column_name(row_cursor, "<CLOSE>");
    if (closeCell == nullptr) {
        goto error;
    }
    const CsvCell* volumeCell =
        get_cell_with_column_name(row_cursor, "<VOL>");
    if (volumeCell == nullptr) {
        goto error;
    }
    auto const header_cells = (StockTableCsvHeaderCells){
        .date = dateCell,
        .open = openCell,
        .high = highCell,
        .low = lowCell,
        .close = closeCell,
        .volume = volumeCell
    };

    // Parse data
    size_t data_index = 0;
    int result = 1;
    while ((result = csv_reader_read_row(&reader)) == 1) {
        if (data_index >= table->capacity) {
            fprintf(
                stderr,
                "Not enough space in the stock data tables; "
                "need %lu more rows\n",
                data_index - table->capacity - 1
            );
            goto error;
        }

        const char* date = dateCell->ptr;
        const uint16_t year = (uint16_t)((date[0] - '0') * 1000 +
            (date[1] - '0') * 100 +
            (date[2] - '0') * 10 +
            (date[3] - '0'));

        if (start_year != NULL && year < *start_year) {
            continue;
        }

        if (end_year != NULL && year >= *end_year) {
            break;
        }

        StockDataRow* current_row = &table->rows[data_index++];
        if (!parse_row(header_cells, current_row, year)) {
            goto error;
        }
    }
    if (result == -1) {
        goto error;
    }

    mapped_file_cursor_clean_up(&file_cursor);
    table->row_count = data_index;

    return true;
error:
    mapped_file_cursor_clean_up(&file_cursor);
    return false;
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
