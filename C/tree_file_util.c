#include "tree_file_util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "internal_config.h"
#include "../CPP/csv_lion/CsvCell.h"
#include "../CPP/csv_lion/CsvCursor.h"
#include "../CPP/csv_lion/CsvReader.h"
#include "../CPP/csv_lion/MappedFileCursor.h"

TreeHashMap* read_tree_csv(const char* filename) {
    MappedFileCursor cursor;
    if (mapped_file_cursor_map_file(&cursor, filename) != 0) {
        return nullptr;
    }

    CsvReader reader;
    csv_reader_init(&reader, &cursor, ',');
    csv_reader_row(&reader);

    const CsvCursor* row = csv_reader_row(&reader);
    csv_reader_read_row(&reader);

    const CsvCell* depth_cell = csv_cursor_with_column_name(row, "Depth");
    const CsvCell* key_cell = csv_cursor_with_column_name(row, "Key");
    const CsvCell* up_cell = csv_cursor_with_column_name(row, "Up");
    const CsvCell* down_cell = csv_cursor_with_column_name(row, "Down");

    if (!depth_cell || !key_cell || !up_cell || !down_cell) {
        fprintf(stderr, "invalid row in tree csv");
        exit(EXIT_FAILURE);
    }


    size_t capacity = 128;
    TreeHashMap** tree_stack = calloc(capacity, sizeof(TreeHashMap*));
    size_t* depth_stack = calloc(capacity, sizeof(size_t));

    TreeHashMap* root = nullptr;

    while (csv_reader_read_row(&reader)) {


        long key;
        extract_long(key_cell->ptr, nullptr, &key);

        uint64_t depth;
        extract_uint64_t(depth_cell->ptr, nullptr, &depth);

        uint64_t up;
        extract_uint64_t(up_cell->ptr, nullptr, &up);

        uint64_t down;
        extract_uint64_t(down_cell->ptr, nullptr, &down);

        TreeHashMap* node = calloc(1, sizeof(TreeHashMap));
        node->key = key;
        node->count_up = up;
        node->count_down = down;
        node->map = calloc(START_MAP_SIZE, sizeof(TreeHashMap*));
        node->size = START_MAP_SIZE;

        // ensure stack can hold current depth
        if (depth + 1 > capacity) {
            capacity *= 2;

            TreeHashMap** tmp_stack =
                realloc(tree_stack, capacity * sizeof(TreeHashMap*));
            if (tmp_stack == NULL) {
                perror("realloc failed");
                exit(EXIT_FAILURE);
            }
            tree_stack = tmp_stack;

            size_t* tmp_depth_stack =
                realloc(depth_stack, capacity * sizeof(size_t));
            if (tmp_depth_stack == NULL) {
                perror("realloc failed");
                exit(EXIT_FAILURE);
            }
            depth_stack = tmp_depth_stack;
        }

        tree_stack[depth] = node;
        depth_stack[depth] = depth;

        if (depth == 0) {
            root = node;
        } else {
            TreeHashMap* parent = tree_stack[depth - 1];
            const size_t parent_depth = depth_stack[depth - 1];
            assert(parent_depth == depth - 1);
            const TreeHashMap* child = get_from_tree_hash_map(parent, key);
            assert(child == NULL);


            uint64_t map_size = parent->size;
            if (__glibc_unlikely(parent->current_size * 2 >= map_size)) {
                resize_hash_map(parent);
                map_size = parent->size;
            }
            size_t i = 0;
            while (parent->map[((size_t)key + (i * i)) % map_size] != NULL) {
                i++;
            }
            const size_t index = ((size_t)key + (i * i)) % map_size;
            parent->map[index] = node;
            parent->current_size++;
        }
    }

    mapped_file_cursor_clean_up(&cursor);
    free(tree_stack);
    free(depth_stack);
    return root;
}

typedef struct {
    const TreeHashMap** nodes;
    size_t* depths;
    size_t count;
    size_t capacity;
} NodeList;

static void add_node(
    NodeList* list,
    const TreeHashMap* node,
    const size_t depth
) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        const TreeHashMap** tmp_nodes =
            realloc(list->nodes, list->capacity * sizeof(TreeHashMap*));
        if (!tmp_nodes) {
            perror("realloc failed");
            exit(EXIT_FAILURE);
        }
        list->nodes = tmp_nodes;

        size_t* tmp_depths =
            realloc(list->depths, list->capacity * sizeof(size_t));
        if (!tmp_depths) {
            perror("realloc failed");
            exit(EXIT_FAILURE);
        }
        list->depths = tmp_depths;
    }
    list->nodes[list->count] = node;
    list->depths[list->count] = depth;
    list->count++;
}

static void dfs(
    const TreeHashMap* node,
    const size_t depth,
    NodeList* list,
    size_t* max_children
) {
    add_node(list, node, depth);
    size_t children = 0;

    for (size_t i = 0; i < node->size; ++i) {
        const TreeHashMap* child = node->map[i];
        if (child) {
            dfs(child, depth + 1, list, max_children);
            children++;
        }
    }

    if (children > *max_children) *max_children = children;
}

void export_tree_to_csv(const TreeHashMap* root, FILE* out) {
    if (!root || !out) return;

    NodeList list = {
        .nodes = malloc(128 * sizeof(TreeHashMap*)),
        .depths = malloc(128 * sizeof(size_t)),
        .count = 0,
        .capacity = 128
    };
    if (!list.nodes || !list.depths) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    size_t max_children = 0;
    dfs(root, 0, &list, &max_children);

    // Write header
    fprintf(out, "Depth,Key,Up,Down");
    for (size_t i = 0; i < max_children; ++i) {
        fprintf(out, ",C%zu", i);
    }
    fprintf(out, "\n");

    // Write rows
    for (size_t i = 0; i < list.count; ++i) {
        const TreeHashMap* node = list.nodes[i];
        const size_t depth = list.depths[i];

        fprintf(
            out,
            "%lu,%ld,%lu,%lu",
            depth,
            node->key,
            node->count_up,
            node->count_down
        );

        // Write child indices
        size_t written = 0;
        for (size_t j = 0; j < node->size; ++j) {
            const TreeHashMap* child = node->map[j];
            if (child) {
                for (size_t k = 0; k < list.count; ++k) {
                    if (list.nodes[k] == child) {
                        fprintf(out, ",%lu", k + 2);
                        break;
                    }
                }
                written++;
            }
        }

        while (written++ < max_children) {
            fprintf(out, ",-1");
        }

        fprintf(out, "\n");
    }

    free(list.nodes);
    free(list.depths);
}

TreeHashMap* load_trees_from_year(const uint16_t year) {
    // filePaths must be freed
    // -------------------------------------------------------------------------
    FilePathList file_data;
    get_all_files_paths_recursive(TREE_DATA_FOLDER, &file_data);
    if (file_data.file_paths == NULL) {
        perror("Error: Could not retrieve file paths.\n");
        exit(1);
    }

    const char* tree_file_path = nullptr;
    for (size_t i = 0; i < file_data.file_count; i++) {
        const char* file_path = file_data.file_paths[i];
        const char* lastSlashPosition = strrchr(file_path, '/');
        assert(lastSlashPosition != NULL);
        const char* file_name = lastSlashPosition + 1;
        char* end;

        // skip "tree_"
        file_name += 5;

        const uint16_t tree_year = (uint16_t)strtol(file_name, &end, 10);
        if (end == file_name) {
            perror("Failed to parse year");
            exit(1);
        }
        if (tree_year == year) {
            tree_file_path = file_path;
            break;
        }
    }

    TreeHashMap* tree = read_tree_csv(tree_file_path);
    free_all_files_paths(&file_data);
    return tree;
}