#include "file_util.h"

#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"

void get_all_file_paths_recursive_helper(
    const char* folder,
    FilePathList* file_path_list,
    DIR* directory
) {
    assert(folder != NULL);
    assert(file_path_list != NULL);
    assert(directory != NULL);

    // Load as many file paths as possible
    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL &&
        file_path_list->file_count < file_path_list->capacity
    ) {
        // Get file path
        char* full_path = nullptr;
        if (asprintf(&full_path, "%s/%s", folder, entry->d_name) == -1) {
            perror("Failed to get file path\n");
            exit(EXIT_FAILURE);
        }

        // Only add if a file; not a directory
        struct stat file_stat;
        if (stat(full_path, &file_stat) != 0) {
            perror("Failed to stat file\n");
            exit(EXIT_FAILURE);
        }
        if (S_ISREG(file_stat.st_mode)) {
            file_path_list->file_paths[file_path_list->file_count] = full_path;
            file_path_list->file_count++;
        } else if (S_ISDIR(file_stat.st_mode) &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0
        ) {
            // sub_dir must be closed
            // -----------------------------------------------------------------
            DIR* sub_dir = opendir(full_path);
            if (sub_dir == NULL) {
                perror("Failed to open directory to get all files paths\n");
                exit(EXIT_FAILURE);
            }

            get_all_file_paths_recursive_helper(
                full_path,
                file_path_list, sub_dir
            );

            // sub_dir closed
            // -----------------------------------------------------------------
            closedir(sub_dir);
            free(full_path);
        } else {
            free(full_path);
        }
    }
}

/**
 * Gets all absolute file paths for all files in the given folder
 * and its subfolders.
 * The caller is responsible for freeing all file paths via freeAllFilesPaths.
 *
 * @param folder The folder to search for files.
 * @param file_path_list A pointer to an empty file path list struct.
 *                       The file paths will be added to this list struct.
 */
void get_all_files_paths_recursive(
    const char* folder,
    FilePathList* file_path_list
) {
    assert(folder != NULL);
    assert(file_path_list != NULL);

    // dir must be closed
    // -------------------------------------------------------------------------
    DIR* dir = opendir(folder);
    if (dir == NULL) {
        perror("Failed to open directory to get all files paths\n");
        exit(EXIT_FAILURE);
    }

    // init FilePathList
    file_path_list->file_paths =
        malloc(NUMBER_OF_STOCK_EXAMPLES * sizeof(char*));
    if (file_path_list->file_paths == NULL) {
        fprintf(stderr, "Failed to allocate space for file paths\n");
        exit(1);
    }
    file_path_list->file_count = 0;
    file_path_list->capacity = NUMBER_OF_STOCK_EXAMPLES;

    get_all_file_paths_recursive_helper(folder, file_path_list, dir);
    // dir closed
    // -------------------------------------------------------------------------
    closedir(dir);
}

/**
 * The caller is responsible for freeing the returned stock data
 * via freeStockDataTables.
 *
 * @param stock_data_tables A pointer to StockDataTables
 *                          which will be filled with stock data
 *                          for the given timeframe specified
 *                          by start_year and end_year.
 * @param start_year The first year, inclusive, of the stock data to be loaded.
 *                   If null, stock data will be loaded from the beginning.
 * @param end_year The last year, exclusive, of the stack data to be loaded.
 *                 If null, stock data will be loaded to the end.
 * @param folder The folder to get stock data from.
 *               All subdirectories will be checked for stock data.
 * @return The results of loading the stock data from disk.
 */
bool load_stock_data_from_disk(
    StockDataTables* stock_data_tables,
    const uint16_t* start_year,
    const uint16_t* end_year,
    const char* folder
) {
    // filePaths must be freed
    // -------------------------------------------------------------------------
    FilePathList file_data;
    get_all_files_paths_recursive(folder, &file_data);
    if (file_data.file_paths == NULL) {
        fprintf(stderr, "Error: Could not retrieve file paths.\n");
        exit(1);
    }

    // init StockDataTables
    const size_t file_count = file_data.file_count;
    stock_data_tables->tables =
        malloc(file_count * sizeof(StockDataTable));
    if (stock_data_tables->tables == NULL) {
        fprintf(stderr, "Failed to allocate stock data tables");
        exit(1);
    }
    stock_data_tables->table_count = 0;
    stock_data_tables->capacity = file_count;

    // Create space for the stock data tables
    for (size_t i = 0; i < stock_data_tables->capacity; i++) {
        StockDataTable* current_table = &stock_data_tables->tables[i];
        constexpr size_t size = LARGEST_STOCK_DATASET_SIZE;
        if (size == 0) {
            current_table->rows = nullptr;
        } else {
            current_table->rows = malloc(size * sizeof(StockDataRow));
            if (current_table->rows == NULL) {
                fprintf(stderr, "Failed to allocate stock data rows");
                exit(1);
            }
        }
        current_table->capacity = size;
        stock_data_tables->table_count++;
    }

    load_stock_data_from_files(
        &file_data,
        stock_data_tables,
        start_year,
        end_year
    );

    // filePaths freed
    // -------------------------------------------------------------------------
    free_all_files_paths(&file_data);
    return true;
}


/**
 * Extracts the stock name from a file path.
 *
 * @param path The path to the stock file.
 * @return The returned string must be freed by the caller.
 */
char* extract_symbol(const char* path) {
    char* symbol = nullptr;
    const char* lastSlashPosition = strrchr(path, '/');
    if (lastSlashPosition != NULL) {
        lastSlashPosition++;
        const char* dotPosition = strchr(lastSlashPosition, '.');
        assert(dotPosition != NULL);

        const long int symbol_length = dotPosition - lastSlashPosition;
        symbol = malloc((size_t)symbol_length + 1); // +1 for null-terminator
        if (symbol == NULL) {
            fprintf(
                stderr,
                "Failed to allocate memory for symbol string"
            );
            exit(EXIT_FAILURE);
        }
        strncpy(symbol, lastSlashPosition, (size_t)symbol_length);
        symbol[symbol_length] = '\0';
    }
    return symbol;
}

/**
 * Loads stock data from files in the file list.
 * The caller must have allocated enough tables in stock_data_tables
 * to match or exceed the number of files in file_path_list.
 *
 * @param file_path_list The list of file paths to load data from.
 * @param stock_data_tables The tables where stock data will be loaded into.
* @param start_year The first year, inclusive,
 *                   of the stock data to be loaded.
 *                   If null, stock data will be loaded from the beginning.
 * @param end_year The last year, exclusive,
 *                 of the stack data to be loaded.
 *                 If null, stock data will be loaded to the end.
 */
void load_stock_data_from_files(
    const FilePathList* file_path_list,
    StockDataTables* stock_data_tables,
    const uint16_t* start_year,
    const uint16_t* end_year
) {
    const size_t file_count = file_path_list->file_count;
    assert(stock_data_tables->capacity >= file_count);
#if IS_PARALLEL
#pragma omp parallel \
for default(none) \
shared(file_count, file_path_list, stock_data_tables, start_year, end_year) \
num_threads(180) \
proc_bind(close)
#endif
    // Load each file into the tables
    for (size_t i = 0; i < file_count; i++) {
        const char* file_name = file_path_list->file_paths[i];
        StockDataTable* table = &stock_data_tables->tables[i];
        table->stock_symbol = extract_symbol(file_name);
        read_stock_csv(
            file_name,
            table,
            start_year,
            end_year
        );
    }

    // Free the zero sized tables and shift the remaining ones over
    size_t write_index = 0;
    const size_t old_table_count = stock_data_tables->table_count;
    for (size_t read_index = 0; read_index < old_table_count; read_index++) {
        StockDataTable* table = &stock_data_tables->tables[read_index];
        if (table->row_count == 0) {
            free(table->stock_symbol);
            free(table->rows);
            continue;
        }

        if (write_index != read_index) {
            stock_data_tables->tables[write_index] = *table;
            table->stock_symbol = nullptr;
            table->rows = nullptr;
        }
        write_index++;
    }
    stock_data_tables->table_count = write_index;
}

/**
 * Frees every file path string in the file path list and the list itself.
 *
 * @param file_path_list The file path list to free.
 */
void free_all_files_paths(FilePathList* file_path_list) {
    for (size_t i = 0; i < file_path_list->file_count; i++) {
        free(file_path_list->file_paths[i]);
    }
    free(file_path_list->file_paths);
    file_path_list->file_count = 0;
    file_path_list->capacity = 0;
}

/**
 * Frees every table and the table list itself.
 *
 * @param stock_data_tables The table list to free.
 */
void free_stock_data_tables(StockDataTables* stock_data_tables) {
    for (
        size_t i = 0;
        i < stock_data_tables->table_count;
        i++
    ) {
        free(stock_data_tables->tables[i].rows);
        free(stock_data_tables->tables[i].stock_symbol);
    }
    free(stock_data_tables->tables);
    free(stock_data_tables);
}
