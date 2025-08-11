#include "tree_file_util.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <bits/fcntl-linux.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "internal_config.h"

size_t get_depth(const char* node) {
    size_t val;
    memcpy(&val, node, sizeof(val));
    return val;
}

long get_key(const char* node) {
    long val;
    memcpy(&val, node + sizeof(size_t), sizeof(val));
    return val;
}

uint64_t get_up_count(const char* node) {
    uint64_t val;
    memcpy(&val, node + sizeof(size_t) + sizeof(long), sizeof(val));
    return val;
}

uint64_t get_down_count(const char* node) {
    uint64_t val;
    memcpy(
        &val,
        node +
        sizeof(size_t) +
        sizeof(long) +
        sizeof(uint64_t),
        sizeof(val)
    );
    return val;
}

uint64_t get_number_of_children(const char* node) {
    uint64_t val;
    memcpy(
        &val,
        node +
        sizeof(size_t) +
        sizeof(long) +
        sizeof(uint64_t) +
        sizeof(uint64_t),
        sizeof(val)
    );
    return val;
}

char* get_child_with_key(char* root, const char* node, const long child_key) {
    const uint64_t number_of_children = get_number_of_children(node);
    const char* start_offset =
        node +
        sizeof(size_t) +
        sizeof(long) +
        sizeof(uint64_t) +
        sizeof(uint64_t);
    for (uint64_t child_index = 0;
         child_index < number_of_children;
         child_index++
    ) {
        const char* key_address =
            start_offset +
            (child_index * (sizeof(long) + sizeof(size_t)));
        long key;
        memcpy(
            &key,
            key_address,
            sizeof(long)
        );
        if (key == child_key) {
            size_t child_offset;
            memcpy(
                &child_offset,
                key_address + sizeof(long),
                sizeof(size_t)
            );
            return  root + child_offset;
        }
    }
    return nullptr;
}

void free_fixed_size_tree(FixedSizeTree* tree) {
    if (tree) {
        munmap(
            tree->start,
            tree->length
        );
        tree->start = NULL;
        tree->length = 0;
    }
}


// File format:
//     A list of these:
//         depth: size_t,
//         key: long,
//         up: uint64_t,
//         down: uint64_t,
//         size: uint64_t,
//             child_key: long
//             child_offset: size_t


FixedSizeTree read_fixed_size_tree_file(const char* filename) {
    const int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        exit(-1);
    }
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("Error getting file stats");
        close(fd);
        exit(-1);
    }

    if (st.st_size == 0) {
        close(fd);
        exit(-1);
    }

    const long int page_size = sysconf(_SC_PAGESIZE);
    const long int length = (st.st_size + page_size - 1) & ~(page_size - 1);
    char* tree_map = mmap(NULL, (size_t)length, PROT_READ, MAP_SHARED, fd, 0);
    if (tree_map == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        exit(-1);
    }
    close(fd);

    const FixedSizeTree tree = {
        .start = tree_map,
        .length = length
    };

    return tree;
}

// TreeHashMap* read_tree_csv(const char* filename) {
//     MappedFileCursor cursor;
//     if (mapped_file_cursor_map_file(&cursor, filename) != 0) {
//         return nullptr;
//     }
//
//     CsvReader reader;
//     csv_reader_init(&reader, &cursor, ',');
//     csv_reader_row(&reader);
//
//     const CsvCursor* row = csv_reader_row(&reader);
//     csv_reader_read_row(&reader);
//
//     const CsvCell* depth_cell = csv_cursor_with_column_name(row, "Depth");
//     const CsvCell* key_cell = csv_cursor_with_column_name(row, "Key");
//     const CsvCell* up_cell = csv_cursor_with_column_name(row, "Up");
//     const CsvCell* down_cell = csv_cursor_with_column_name(row, "Down");
//
//     if (!depth_cell || !key_cell || !up_cell || !down_cell) {
//         fprintf(stderr, "invalid row in tree csv");
//         exit(EXIT_FAILURE);
//     }
//
//
//     size_t capacity = 128;
//     TreeHashMap** tree_stack = calloc(capacity, sizeof(TreeHashMap*));
//     size_t* depth_stack = calloc(capacity, sizeof(size_t));
//
//     TreeHashMap* root = nullptr;
//
//     while (csv_reader_read_row(&reader)) {
//
//
//         long key;
//         extract_long(key_cell->ptr, nullptr, &key);
//
//         uint64_t depth;
//         extract_uint64_t(depth_cell->ptr, nullptr, &depth);
//
//         uint64_t up;
//         extract_uint64_t(up_cell->ptr, nullptr, &up);
//
//         uint64_t down;
//         extract_uint64_t(down_cell->ptr, nullptr, &down);
//
//         TreeHashMap* node = calloc(1, sizeof(TreeHashMap));
//         node->key = key;
//         node->count_up = up;
//         node->count_down = down;
//         node->map = calloc(START_MAP_SIZE, sizeof(TreeHashMap*));
//         node->size = START_MAP_SIZE;
//
//         // ensure stack can hold current depth
//         if (depth + 1 > capacity) {
//             capacity *= 2;
//
//             TreeHashMap** tmp_stack =
//                 realloc(tree_stack, capacity * sizeof(TreeHashMap*));
//             if (tmp_stack == NULL) {
//                 perror("realloc failed");
//                 exit(EXIT_FAILURE);
//             }
//             tree_stack = tmp_stack;
//
//             size_t* tmp_depth_stack =
//                 realloc(depth_stack, capacity * sizeof(size_t));
//             if (tmp_depth_stack == NULL) {
//                 perror("realloc failed");
//                 exit(EXIT_FAILURE);
//             }
//             depth_stack = tmp_depth_stack;
//         }
//
//         tree_stack[depth] = node;
//         depth_stack[depth] = depth;
//
//         if (depth == 0) {
//             root = node;
//         } else {
//             TreeHashMap* parent = tree_stack[depth - 1];
//             const size_t parent_depth = depth_stack[depth - 1];
//             assert(parent_depth == depth - 1);
//             const TreeHashMap* child = get_from_tree_hash_map(parent, key);
//             assert(child == NULL);
//
//
//             uint64_t map_size = parent->size;
//             if (__glibc_unlikely(parent->current_size * 2 >= map_size)) {
//                 resize_hash_map(parent);
//                 map_size = parent->size;
//             }
//             size_t i = 0;
//             while (parent->map[((size_t)key + (i * i)) % map_size] != NULL) {
//                 i++;
//             }
//             const size_t index = ((size_t)key + (i * i)) % map_size;
//             parent->map[index] = node;
//             parent->current_size++;
//         }
//     }
//
//     mapped_file_cursor_clean_up(&cursor);
//     free(tree_stack);
//     free(depth_stack);
//     return root;
// }

typedef struct {
    const TreeHashMap** nodes;
    size_t* depths;
    size_t count;
    size_t capacity;
} NodeList;

static void add_to_node_list(
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
    add_to_node_list(list, node, depth);
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

int is_little_endian() {
    uint16_t x = 1;
    return *((uint8_t*)&x) == 1;
}

void check_little_endian() {
    if (!is_little_endian()) {
        perror("Big endian not implemented");
        exit(-1);
    }
}

void export_tree_to_file(const TreeHashMap* root, FILE* out) {
    check_little_endian();
    if (!root || !out) return;

    NodeList node_list = {
        .nodes = malloc(128 * sizeof(TreeHashMap*)),
        .depths = malloc(128 * sizeof(size_t)),
        .count = 0,
        .capacity = 128
    };
    if (!node_list.nodes || !node_list.depths) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    size_t max_children = 0;
    dfs(root, 0, &node_list, &max_children);

    size_t* offsets = calloc(node_list.count, sizeof(size_t));
    size_t current_offset = 0;
    // Write nodes
    for (size_t i = 0; i < node_list.count; ++i) {
        offsets[i] = current_offset;
        const TreeHashMap* node = node_list.nodes[i];
        const size_t depth = node_list.depths[i];
        fwrite(&depth, sizeof(depth), 1, out);
        fwrite(&node->key, sizeof(node->key), 1, out);
        fwrite(&node->count_up, sizeof(node->count_up), 1, out);
        fwrite(&node->count_down, sizeof(node->count_down), 1, out);
        fwrite(&node->current_size, sizeof(node->current_size), 1, out);

        // Write child indices
        size_t written = 0;
        for (size_t j = 0; j < node->size; ++j) {
            const TreeHashMap* child = node->map[j];
            if (child) {
                for (size_t k = 0; k < node_list.count; ++k) {
                    if (node_list.nodes[k] == child) {
                        fwrite(&child->key, sizeof(child->key), 1, out);
                        fwrite(&k, sizeof(k), 1, out);
                        break;
                    }
                }
                written++;
            }
        }
        current_offset +=
            sizeof(depth) +
            sizeof(node->key) +
            sizeof(node->count_up) +
            sizeof(node->count_down) +
            sizeof(node->current_size) +
            (written * (sizeof(size_t) + sizeof(long)));
    }
    fflush(out);
    fseek(out, 0, SEEK_SET);
    current_offset = 0;
    for (size_t i = 0; i < node_list.count; ++i) {
        current_offset += sizeof(node_list.depths[i]) +
            sizeof(node_list.nodes[i]->key) +
            sizeof(node_list.nodes[i]->count_up) +
            sizeof(node_list.nodes[i]->count_down);
        fseek(out, current_offset, SEEK_SET);
        uint64_t current_size;
        current_offset += fread(
            &current_size,
            sizeof(node_list.nodes[i]->current_size),
            1,
            out
        );

        for (uint64_t j = 0; j < current_size; j++) {
            size_t child_key;
            fread(
                &child_key,
                sizeof(child_key),
                1,
                out
            );
            size_t child_offset;
            fread(
                &child_offset,
                sizeof(child_offset),
                1,
                out
            );
            fseek(out, -sizeof(child_offset), SEEK_CUR);
            current_offset +=
                fwrite(
                    &(offsets[child_offset]),
                    sizeof(offsets[child_offset]),
                    1,
                    out
                );
        }
    }
    free(node_list.nodes);
    free(node_list.depths);
}


// void export_tree_to_csv(const TreeHashMap* root, FILE* out) {
//     if (!root || !out) return;
//
//     NodeList list = {
//         .nodes = malloc(128 * sizeof(TreeHashMap*)),
//         .depths = malloc(128 * sizeof(size_t)),
//         .count = 0,
//         .capacity = 128
//     };
//     if (!list.nodes || !list.depths) {
//         perror("malloc failed");
//         exit(EXIT_FAILURE);
//     }
//
//     size_t max_children = 0;
//     dfs(root, 0, &list, &max_children);
//
//     // Write header
//     fprintf(out, "Depth,Key,Up,Down");
//     for (size_t i = 0; i < max_children; ++i) {
//         fprintf(out, ",C%zu", i);
//     }
//     fprintf(out, "\n");
//
//     // Write rows
//     for (size_t i = 0; i < list.count; ++i) {
//         const TreeHashMap* node = list.nodes[i];
//         const size_t depth = list.depths[i];
//
//         fprintf(
//             out,
//             "%lu,%ld,%lu,%lu",
//             depth,
//             node->key,
//             node->count_up,
//             node->count_down
//         );
//
//         // Write child indices
//         size_t written = 0;
//         for (size_t j = 0; j < node->size; ++j) {
//             const TreeHashMap* child = node->map[j];
//             if (child) {
//                 for (size_t k = 0; k < list.count; ++k) {
//                     if (list.nodes[k] == child) {
//                         fprintf(out, ",%lu", k + 2);
//                         break;
//                     }
//                 }
//                 written++;
//             }
//         }
//
//         while (written++ < max_children) {
//             fprintf(out, ",-1");
//         }
//
//         fprintf(out, "\n");
//     }
//
//     free(list.nodes);
//     free(list.depths);
// }

FixedSizeTree load_trees_from_year(const uint16_t year) {
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

    FixedSizeTree tree = read_fixed_size_tree_file(tree_file_path);
    free_all_files_paths(&file_data);
    return tree;
}
