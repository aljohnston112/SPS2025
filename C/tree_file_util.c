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

// File format:
//     A list of these:
//         depth: size_t,
//         key: long,
//         up: uint64_t,
//         down: uint64_t,
//         size: uint64_t,
//             child_key: long
//             child_offset: size_t

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
            return root + child_offset;
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
            sizeof(current_size),
            1,
            out
        ) * sizeof(current_size);
        for (uint64_t j = 0; j < current_size; j++) {
            size_t child_key;
            current_offset += fread(
                &child_key,
                sizeof(child_key),
                1,
                out
            ) * sizeof(child_key);

            size_t child_index;
            fread(
                &child_index,
                sizeof(child_index),
                1,
                out
            );
            fseek(out, -sizeof(child_index), SEEK_CUR);
            current_offset +=
                fwrite(
                    &(offsets[child_index]),
                    sizeof(offsets[child_index]),
                    1,
                    out
                ) * sizeof(offsets[child_index]);
            fflush(out);
        }
    }
    free(node_list.nodes);
    free(node_list.depths);
}

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
