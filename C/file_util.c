#include "file_util.h"

#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"

bool get_all_file_paths_recursive_helper(
    const char* folder,
    FilePathList* file_path_list,
    DIR* directory
) {
    if (folder == NULL) {
        fprintf(stderr, "folder was null");
        return false;
    }
    if (file_path_list == NULL) {
        fprintf(stderr, "file path list was null");
        return false;
    }
    if (directory == NULL) {
        fprintf(stderr, "directory was null");
        return false;
    }

    // Load as many file paths as possible
    struct dirent* entry = nullptr;
    while ((entry = readdir(directory)) != NULL &&
        file_path_list->file_count < file_path_list->capacity
    ) {
        if (
            strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0
        ) {
            continue;
        }

        // Get file path
        char* full_path = nullptr;
        if (asprintf(&full_path, "%s/%s", folder, entry->d_name) == -1) {
            perror("Failed to get file path\n");
            file_path_list->file_count = 0;
            for (size_t i = 0; i < file_path_list->file_count; i++) {
                free((void *)(uintptr_t)file_path_list->file_paths[i]);
            }
            return false;
        }

        // Only add if a file; not a directory
        struct stat file_stat;
        if (stat(full_path, &file_stat) != 0) {
            perror("Failed to stat file\n");
            free(full_path);
            continue;
        }
        if (S_ISREG(file_stat.st_mode)) {
            file_path_list->file_paths[file_path_list->file_count] = full_path;
            file_path_list->file_count++;
        } else if (S_ISDIR(file_stat.st_mode)) {
            // sub_dir must be closed
            // -----------------------------------------------------------------
            DIR* sub_dir = opendir(full_path);
            if (sub_dir == NULL) {
                perror("Failed to open directory to get all files paths\n");
                free(full_path);
                continue;
            }

            if (!get_all_file_paths_recursive_helper(
                    full_path,
                    file_path_list,
                    sub_dir
                )
            ) {
                free(full_path);
                closedir(sub_dir);
                for (size_t i = 0; i < file_path_list->file_count; i++) {
                    free((void *)(uintptr_t)file_path_list->file_paths[i]);
                }
                file_path_list->file_count = 0;
                return false;;
            }
            // sub_dir closed
            // -----------------------------------------------------------------
            closedir(sub_dir);
            free(full_path);
        } else {
            free(full_path);
        }
    }
    return true;
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
bool get_all_files_paths_recursive(
    const char* folder,
    FilePathList* file_path_list
) {
    if (folder == NULL) {
        fprintf(stderr, "folder was null");
        return false;
    }
    if (file_path_list == NULL) {
        fprintf(stderr, "file path list was null");
        return false;
    }

    // init FilePathList
    file_path_list->file_paths =
        malloc(NUMBER_OF_STOCK_EXAMPLES * sizeof(char*));
    if (file_path_list->file_paths == NULL) {
        fprintf(stderr, "Failed to allocate space for file paths\n");
        return false;
    }
    file_path_list->file_count = 0;
    file_path_list->capacity = NUMBER_OF_STOCK_EXAMPLES;

    // dir must be closed
    // -------------------------------------------------------------------------
    DIR* dir = opendir(folder);
    if (dir == NULL) {
        perror("Failed to open directory to get all files paths\n");
        free(file_path_list->file_paths);
        return false;
    }

    if (!get_all_file_paths_recursive_helper(folder, file_path_list, dir)) {
        closedir(dir);
        free(file_path_list->file_paths);
        return false;
    }
    // dir closed
    // -------------------------------------------------------------------------
    closedir(dir);
    return true;
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
    if (!get_all_files_paths_recursive(folder, &file_data)) {
        return false;
    }

    // init StockDataTables
    const size_t file_count = file_data.file_count;
    stock_data_tables->tables =
        calloc(file_count, sizeof(StockDataTable));
    if (stock_data_tables->tables == NULL) {
        free_all_files_paths(&file_data);
        fprintf(stderr, "Failed to allocate stock data tables");
        return false;
    }
    stock_data_tables->table_count = 0;
    stock_data_tables->capacity = file_count;

    // Create space for the stock data tables
    for (size_t i = 0; i < stock_data_tables->capacity; i++) {
        StockDataTable* current_table = &stock_data_tables->tables[i];
        constexpr size_t size = LARGEST_STOCK_DATASET_SIZE;
        current_table->rows = calloc(size, sizeof(StockDataRow));
        if (current_table->rows == NULL) {
            free_stock_data_tables(stock_data_tables);
            free_all_files_paths(&file_data);
            fprintf(stderr, "Failed to allocate stock data rows");
            return false;
        }
        current_table->capacity = size;
        stock_data_tables->table_count++;
    }

    if (!load_stock_data_from_files(
            &file_data,
            stock_data_tables,
            start_year,
            end_year
        )
    ) {
        free_all_files_paths(&file_data);
        return false;
    }

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
    const char* lastSlashPosition = strrchr(path, '/');
    if (lastSlashPosition == NULL) {
        fprintf(
            stderr,
            "No slash in file path"
        );
        return nullptr;
    }
    lastSlashPosition++;

    const char* dotPosition = strchr(lastSlashPosition, '.');
    if (dotPosition == NULL) {
        fprintf(
            stderr,
            "No dot in file path"
        );
        return nullptr;
    }

    const long int symbol_length = dotPosition - lastSlashPosition;
    char* symbol = malloc((size_t)symbol_length + 1); // +1 for null-terminator
    if (symbol == NULL) {
        fprintf(
            stderr,
            "Failed to allocate memory for symbol string"
        );
        return nullptr;
    }
    strncpy(symbol, lastSlashPosition, (size_t)symbol_length);
    symbol[symbol_length] = '\0';
    return symbol;
}

typedef struct {
    const char** file_paths;
    StockDataTable* tables;
    size_t start_i;
    size_t end_i;
    const uint16_t* start_year;
    const uint16_t* end_year;
    bool success;
} load_stock_data_thread_metadata;

__attribute__((noreturn)) void* load_stock_data_thread_routine(void* args_in) {
    auto const args = (load_stock_data_thread_metadata*)args_in;
    for (size_t i = args->start_i; i < args->end_i; i++) {
        const char* file_name = args->file_paths[i];
        StockDataTable* table = &args->tables[i];
        table->stock_symbol = extract_symbol(file_name);
        if (!read_stock_csv(
            file_name,
            table,
            args->start_year,
            args->end_year
        )) {
            args->success = false;
            pthread_exit(&args->success);
        }
    }
    args->success = true;
    pthread_exit(&args->success);
}

bool old_read_csv(
    const FilePathList* file_path_list,
    StockDataTables* stock_data_tables,
    const uint16_t* start_year,
    const uint16_t* end_year,
    const size_t file_count
) {
    /*
    #if IS_PARALLEL
    #pragma omp parallel \
    for default(none) \
    shared(file_count, file_path_list, stock_data_tables, start_year, end_year) \
    num_threads(180) \
    proc_bind(close)
    #endif
    */
    // Load each file into the tables
    for (size_t i = 0; i < file_count; i++) {
        const char* file_name = file_path_list->file_paths[i];
        StockDataTable* table = &stock_data_tables->tables[i];
        table->stock_symbol = extract_symbol(file_name);
        if (!read_stock_csv(
                file_name,
                table,
                start_year,
                end_year
            )
        ) {
            free_stock_data_tables(stock_data_tables);
            return false;
        }
    }

    return true;
}

/**
 * Loads stock data from files in the file list.
 * The caller must have allocated enough tables in stock_data_tables
 * to match or exceed the number of files in file_path_list.
 *
 * @param file_path_list The list of file paths to load data from.
 * @param stock_data_tables The tables where stock data will be loaded into.
 *                          The rows in the table must be allocated by the calller.
* @param start_year The first year, inclusive,
 *                   of the stock data to be loaded.
 *                   If null, stock data will be loaded from the beginning.
 * @param end_year The last year, exclusive,
 *                 of the stack data to be loaded.
 *                 If null, stock data will be loaded to the end.
 */
bool load_stock_data_from_files(
    const FilePathList* file_path_list,
    StockDataTables* stock_data_tables,
    const uint16_t* start_year,
    const uint16_t* end_year
) {
    const size_t file_count = file_path_list->file_count;
    if (stock_data_tables->capacity < file_count) {
        free_stock_data_tables(stock_data_tables);
        return false;
    }

#ifdef PARALLEL
        constexpr uint8_t number_of_threads = 6;
        pthread_t* threads = malloc(number_of_threads * sizeof(pthread_t));
        if (threads == nullptr) {
            perror("Failed to allocate threads");
            free_stock_data_tables(stock_data_tables);
            return false;
        }

        load_stock_data_thread_metadata* thread_args = malloc(
            number_of_threads * sizeof(load_stock_data_thread_metadata)
        );
        if (thread_args == NULL) {
            perror("Failed to allocate thread args");
            free_stock_data_tables(stock_data_tables);
            free(threads);
            return false;
        }

    const size_t chunk_size = file_path_list->file_count / number_of_threads;
    for (uint8_t i = 0; i < number_of_threads; i++) {
        thread_args[i] = (load_stock_data_thread_metadata){
            .file_paths = file_path_list->file_paths,
            .tables = stock_data_tables->tables,
            .start_i = i * chunk_size,
            .end_i = (i == number_of_threads - 1)
                         ? file_path_list->file_count
                         : (i + 1) * chunk_size - 1,
            .start_year = start_year,
            .end_year = end_year,
            .success = false
        };
        if (pthread_create(
                &threads[i],
                NULL,
                load_stock_data_thread_routine,
                &thread_args[i]
            ) != 0
        ) {
            perror("Failed to create thread");
            free_stock_data_tables(stock_data_tables);
            free(threads);
            free(thread_args);
            return false;
        }
    }

    for (uint8_t i = 0; i < number_of_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            free_stock_data_tables(stock_data_tables);
            free(threads);
            free(thread_args);
            return false;
        }
        if (!thread_args[i].success) {
            perror("thread failed");
            free_stock_data_tables(stock_data_tables);
            free(threads);
            free(thread_args);
            return false;
        }
    }
#else
    if (!old_read_csv(
        file_path_list,
        stock_data_tables,
        start_year,
        end_year,
        file_count
    )
    ) {
        free_stock_data_tables(stock_data_tables);
        return false;
    }
#endif



    // Free the zero sized tables and shift the remaining ones over
    size_t write_index = 0;
    const size_t old_table_count = stock_data_tables->table_count;
    for (size_t read_index = 0; read_index < old_table_count; read_index++) {
        StockDataTable* table = &stock_data_tables->tables[read_index];
        if (table->row_count == 0) {
            free((void *)(uintptr_t)table->stock_symbol);
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
    return true;
}

/**
 * Frees every file path string in the file path list and the list itself.
 *
 * @param file_path_list The file path list to free.
 */
void free_all_files_paths(FilePathList* file_path_list) {
    for (size_t i = 0; i < file_path_list->file_count; i++) {
        free((void *)(uintptr_t)file_path_list->file_paths[i]);
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
        free((void *)(uintptr_t)stock_data_tables->tables[i].stock_symbol);
    }
    free(stock_data_tables->tables);
    free(stock_data_tables);
}
