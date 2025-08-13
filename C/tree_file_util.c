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

#include "config.h"
#include "file_util.h"
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
    assert(filename != nullptr);
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
        perror("File is empty");
        close(fd);
        exit(-1);
    }

    const long int page_size = sysconf(_SC_PAGESIZE);
    const long int length = (st.st_size + page_size - 1) & ~(page_size - 1);
    char* tree_map = mmap(
        NULL,
        (size_t)length,
        PROT_READ,
        MAP_SHARED,
        fd,
        0
    );
    if (tree_map == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        exit(-1);
    }
    close(fd);

    const FixedSizeTree tree = {
        .start = tree_map,
        .length = (size_t)length
    };

    return tree;
}

typedef struct {
    const SequenceCountingTrie** nodes;
    size_t* depths;
    size_t count;
    size_t capacity;
} NodeList;

static void add_to_node_list(
    NodeList* list,
    const SequenceCountingTrie* node,
    const size_t depth
) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        const SequenceCountingTrie** tmp_nodes =
            realloc(list->nodes, list->capacity * sizeof(SequenceCountingTrie*));
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
    const SequenceCountingTrie* node,
    const size_t depth,
    NodeList* list,
    size_t* number_of_nodes
) {
    add_to_node_list(list, node, depth);
    (*number_of_nodes)++;

    for (size_t i = 0; i < node->capacity; ++i) {
        const SequenceCountingTrie* child = node->map[i];
        if (child) {
            dfs(child, depth + 1, list, number_of_nodes);
        }
    }
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

typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
} Buffer;

void ensure_buffer_capacity(
    Buffer* buffer,
    const size_t needed
) {
    if (needed > buffer->capacity) {
        size_t new_capacity = buffer->capacity ? buffer->capacity : 64;
        while (new_capacity < needed) {
            new_capacity *= 2;
        }
        uint8_t* new_data = realloc(buffer->data, new_capacity);
        if (!new_data) {
            perror("realloc failed");
            exit(EXIT_FAILURE);
        }
        buffer->data = new_data;
        buffer->capacity = new_capacity;
    }
}

void export_tree_to_file(const SequenceCountingTrie* root, FILE* out) {
    check_little_endian();
    if (!root || !out) return;

    // Convert the tree to a list of nodes
    NodeList node_list = {
        .nodes = malloc(128 * sizeof(SequenceCountingTrie*)),
        .depths = malloc(128 * sizeof(size_t)),
        .count = 0,
        .capacity = 128
    };
    if (!node_list.nodes || !node_list.depths) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    size_t number_of_nodes = 0;
    dfs(root, 0, &node_list, &number_of_nodes);


    // Write nodes
    // 40 bytes per node is a vast underestimate
    const size_t buffer_capacity = 40 * number_of_nodes;
    Buffer buffer = {
        .data = malloc(buffer_capacity),
        .size = 0,
        .capacity = buffer_capacity
    };

    size_t* offsets = calloc(node_list.count, sizeof(size_t));
    size_t current_offset = 0;
    for (size_t i = 0; i < node_list.count; ++i) {
        offsets[i] = current_offset;
        const SequenceCountingTrie* node = node_list.nodes[i];
        const size_t depth = node_list.depths[i];

        ensure_buffer_capacity(
            &buffer,
            current_offset +
            sizeof(depth) +
            sizeof(node->key) +
            sizeof(node->count_up) +
            sizeof(node->count_down) +
            sizeof(node->current_size)
        );

        memcpy(&buffer.data[current_offset], &depth, sizeof(depth));
        current_offset += sizeof(depth);
        memcpy(&buffer.data[current_offset], &node->key, sizeof(node->key));
        current_offset += sizeof(node->key);
        memcpy(
            &buffer.data[current_offset],
            &node->count_up,
            sizeof(node->count_up)
        );
        current_offset += sizeof(node->count_up);
        memcpy(
            &buffer.data[current_offset],
            &node->count_down,
            sizeof(node->count_down)
        );
        current_offset += sizeof(node->count_down);
        memcpy(
            &buffer.data[current_offset],
            &node->current_size,
            sizeof(node->current_size)
        );
        current_offset += sizeof(node->current_size);
        buffer.size = current_offset;


        // Write child indices
        for (size_t j = 0; j < node->capacity; ++j) {
            const SequenceCountingTrie* child = node->map[j];
            if (child) {
                for (size_t k = 0; k < node_list.count; ++k) {
                    if (node_list.nodes[k] == child) {
                        ensure_buffer_capacity(
                            &buffer,
                            current_offset +
                            sizeof(child->key) +
                            sizeof(k)
                        );
                        memcpy(
                            &buffer.data[current_offset],
                            &child->key,
                            sizeof(child->key)
                        );
                        current_offset += sizeof(child->key);
                        memcpy(
                            &buffer.data[current_offset],
                            &k,
                            sizeof(k)
                        );
                        current_offset += sizeof(k);
                        buffer.size = current_offset;
                        break;
                    }
                }
            }
        }
    }
    current_offset = 0;
    for (size_t i = 0; i < node_list.count; ++i) {
        current_offset += sizeof(node_list.depths[i]) +
            sizeof(node_list.nodes[i]->key) +
            sizeof(node_list.nodes[i]->count_up) +
            sizeof(node_list.nodes[i]->count_down);
        const uint64_t current_size =
            *(u_int64_t*)(buffer.data + current_offset);
        current_offset += sizeof(current_size);
        for (uint64_t j = 0; j < current_size; j++) {
            // Skip the child key
            current_offset += sizeof(size_t);
            const size_t child_index =
                *((size_t*)(buffer.data + current_offset));
            (*(size_t*)(buffer.data + current_offset)) = offsets[child_index];
            current_offset += sizeof(child_index);
        }
    }
    fwrite(buffer.data, 1, buffer.size, out);
    fflush(out);
    free(buffer.data);
    free(node_list.nodes);
    free(node_list.depths);
    free(offsets);
}

FixedSizeTree load_tree_from_year(const uint16_t year) {
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

    const FixedSizeTree tree = read_fixed_size_tree_file(tree_file_path);

    // filePaths  freed
    // -------------------------------------------------------------------------
    free_all_files_paths(&file_data);
    return tree;
}

uint64_t get_number_of_nodes(const char* tree) {
    const uint64_t number_of_children = get_number_of_children(tree);
    uint64_t number_of_nodes = 1;
    const char* start_offset =
        tree +
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
        const char* child_address = key_address + sizeof(long);
        number_of_nodes +=
            get_number_of_nodes(tree + (*(const size_t*)child_address));
    }
    return number_of_nodes;
}

uint64_t get_max_node_size(char* tree) {
    const uint64_t number_of_children = get_number_of_children(tree);
    uint64_t number_of_bytes =
        sizeof(size_t) +
        sizeof(long) +
        sizeof(uint64_t) +
        sizeof(uint64_t) +
        sizeof(uint64_t) +
        (number_of_children * (sizeof(long) + sizeof(size_t)));
    const char* start_offset =
        tree +
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
        const char* child_address = key_address + sizeof(long);
        const uint64_t child_number_of_bytes =
            get_max_node_size(tree + (*(const size_t*)child_address));
        if (child_number_of_bytes > number_of_bytes) {
            number_of_bytes = child_number_of_bytes;
        }
    }
    return number_of_bytes;
}

void print_bounds_on_trees() {
    for (uint16_t year = START_YEAR; year < END_YEAR; year++) {
        // tree must be freed
        // ---------------------------------------------------------------------
        FixedSizeTree tree = load_tree_from_year(year);
        printf(
            "Number of nodes for %hu: %lu\n",
            year,
            get_number_of_nodes(tree.start)
        );

        printf(
            "Max node size for %hu: %lu\n",
            year,
            get_max_node_size(tree.start)
        );

        // tree freed
        // ---------------------------------------------------------------------
        free_fixed_size_tree(&tree);
    }
}
