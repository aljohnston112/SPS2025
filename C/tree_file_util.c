#include "tree_file_util.h"

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
//         A size sized list of these:
//             child_key: long
//             child_offset: size_t

constexpr int depth_offset = 0;
constexpr int depth_size = sizeof(size_t);

constexpr int key_offset = depth_size;
constexpr int key_size = sizeof(long);

constexpr int up_count_offset = key_offset + key_size;
constexpr int up_count_size = sizeof(uint64_t);

constexpr int down_count_offset = up_count_offset + up_count_size;
constexpr int down_count_size = sizeof(uint64_t);

constexpr int number_of_children_offset = down_count_offset + down_count_size;
constexpr int number_of_children_size = sizeof(uint64_t);

constexpr int child_list_offset =
    number_of_children_offset + number_of_children_size;

constexpr int child_offset_size = sizeof(size_t);
constexpr int child_entry_size = key_size + child_offset_size;

/**
 * @param node A pointer to the start of a trie node.
 * @return The depth of the pointed to node.
 */
size_t get_depth(const char* node) {
    size_t val;
    memcpy(&val, node + depth_offset, depth_size);
    return val;
}

/**
 * @param node A pointer to the start of a trie node.
 * @return The key of the pointed to node.
 */
long get_key(const char* node) {
    long val;
    memcpy(&val, node + key_offset, key_size);
    return val;
}

/**
 * @param node A pointer to the start of a trie node.
 * @return The up count of the pointed to node.
 */
uint64_t get_up_count(const char* node) {
    uint64_t val;
    memcpy(&val, node + up_count_offset, up_count_size);
    return val;
}

/**
 * @param node A pointer to the start of a trie node.
 * @return The down count of the pointed to node.
 */
uint64_t get_down_count(const char* node) {
    uint64_t val;
    memcpy(&val, node + down_count_offset, down_count_size);
    return val;
}

/**
 * @param node A pointer to the start of a trie node.
 * @return The number of children the pointed to node has.
 */
uint64_t get_number_of_children(const char* node) {
    uint64_t val;
    memcpy(
        &val,
        node +
        number_of_children_offset,
        number_of_children_size
    );
    return val;
}

/**
 * Gets the child of the given node with the given key.
 * 
 * @param root The root of the entire trie.
 *             This is needed since offset are from the root start address.
 * @param node The pointer to the start of the node to get the child of.
 * @param child_key The key of the child to get from node.
 * @return The start address of the child node of node with the given key, or
 *         null if node does not have a child with the given key.
 */
char* get_child_with_key(char* root, const char* node, const long child_key) {
    const uint64_t number_of_children = get_number_of_children(node);
    const char* start_offset = node + child_list_offset;
    for (uint64_t child_index = 0;
         child_index < number_of_children;
         child_index++
    ) {
        const char* key_address =
            start_offset + (child_index * child_entry_size);
        long key;
        memcpy(
            &key,
            key_address,
            key_size
        );
        if (key == child_key) {
            size_t child_offset;
            memcpy(
                &child_offset,
                key_address + sizeof(long),
                child_offset_size
            );
            return root + child_offset;
        }
    }
    return nullptr;
}

/**
 *
 * @return true if the system is little endian, else false.
 */
int is_little_endian() {
    uint16_t x = 1;
    return *((uint8_t*)&x) == 1;
}

/**
 * Ends the program if the system is big endian.
 */
void check_little_endian() {
    if (!is_little_endian()) {
        fprintf(stderr, "Big endian not implemented");
        exit(EXIT_FAILURE);
    }
}

typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
} Buffer;

/**
 * Checks the capacity of the given buffer,
 * and if there are not at least "needed" bytes of capacity,
 * capacity will be expanded.
 * If expansion fails, the program will exit.
 *
 * @param buffer The buffer to ensure capacity of.
 * @param needed How many bytes the buffer capacity should be.
 */
void ensure_buffer_capacity(
    Buffer* buffer,
    const size_t needed
) {
    if (needed > buffer->capacity) {
        size_t new_capacity = buffer->capacity ? buffer->capacity : 128;
        while (new_capacity < needed) {
            new_capacity *= 2;
        }
        uint8_t* new_data = realloc(buffer->data, new_capacity);
        if (!new_data) {
            fprintf(stderr, "realloc failed");
            exit(EXIT_FAILURE);
        }
        buffer->data = new_data;
        buffer->capacity = new_capacity;
    }
}

typedef struct {
    const SequenceCountingTrie** nodes;
    size_t* depths;
    size_t count;
    size_t capacity;
} NodeList;

/**
 * Adds a node to the given node list.
 * If the node cannot be added to the given list
 * due to system space running out, the program will exit.
 *
 * @param list The node list to add a node to.
 * @param node The node to add to the given node list.
 * @param depth The depth of the node to add to the given node list.
 */
void add_to_node_list(
    NodeList* list,
    const SequenceCountingTrie* node,
    const size_t depth
) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        const SequenceCountingTrie** new_nodes = realloc(
            list->nodes,
            list->capacity * sizeof(SequenceCountingTrie*)
        );
        if (!new_nodes) {
            fprintf(stderr, "realloc failed");
            exit(EXIT_FAILURE);
        }
        list->nodes = new_nodes;

        size_t* new_depths =
            realloc(list->depths, list->capacity * sizeof(size_t));
        if (!new_depths) {
            fprintf(stderr, "realloc failed");
            exit(EXIT_FAILURE);
        }
        list->depths = new_depths;
    }
    list->nodes[list->count] = node;
    list->depths[list->count] = depth;
    list->count++;
}

/**
 * Builds a node list by crawling the given trie in depth-first fashion
 * and adding the visited nodes to the list in the order they are visited.
 *
 * @param trie The trie to get a flattened version of.
 * @param depth The depth of the given trie.
 * @param list The node list to add to.
 */
void get_node_list_of_trie(
    const SequenceCountingTrie* trie,
    const size_t depth,
    NodeList* list
) {
    add_to_node_list(list, trie, depth);

    for (size_t i = 0; i < trie->capacity; ++i) {
        const SequenceCountingTrie* child = trie->map[i];
        if (child) {
            get_node_list_of_trie(child, depth + 1, list);
        }
    }
}

/**
 * Writes the given trie to file.
 * The file format of the trie is as follows:
 *    A list of these:
 *        depth: size_t,
 *        key: long,
 *        up_count: uint64_t,
 *        down_count: uint64_t,
 *        number_of_children: uint64_t,
 *        A number_of_children sized list of these:
 *            child_key: long
 *            child_offset: size_t
 *
 * This method will fail if the system is little endian.
 *
 * @param trie The trie to write to file.
 * @param output_file The file to output the tree.
 */
void export_tree_to_file(const SequenceCountingTrie* trie, FILE* output_file) {
    assert(trie);
    assert(output_file);
    check_little_endian();

    // Convert the tree to a list of nodes
    static int start_size = 128;
    NodeList node_list = {
        .nodes = malloc(start_size * sizeof(SequenceCountingTrie*)),
        .depths = malloc(start_size * sizeof(size_t)),
        .count = 0,
        .capacity = start_size
    };
    if (!node_list.nodes || !node_list.depths) {
        fprintf(stderr, "malloc failed");
        exit(EXIT_FAILURE);
    }

    get_node_list_of_trie(trie, 0, &node_list);

    // Write nodes
    // 40 bytes per node is a vast underestimate
    const size_t buffer_capacity = 40 * node_list.count;
    Buffer buffer = {
        .data = malloc(buffer_capacity),
        .size = 0,
        .capacity = buffer_capacity
    };
    if (!buffer.data) {
        fprintf(stderr, "malloc failed");
        exit(EXIT_FAILURE);
    }

    size_t* offsets = calloc(node_list.count, sizeof(size_t));
    if (!offsets) {
        fprintf(stderr, "malloc failed");
        exit(EXIT_FAILURE);
    }

    size_t current_offset = 0;
    for (size_t i = 0; i < node_list.count; ++i) {
        offsets[i] = current_offset;
        const SequenceCountingTrie* node = node_list.nodes[i];
        const size_t depth = node_list.depths[i];

        ensure_buffer_capacity(
            &buffer,
            current_offset + child_list_offset
        );

        // write everything but the children
        memcpy(&buffer.data[current_offset], &depth, depth_size);
        current_offset += depth_size;
        memcpy(&buffer.data[current_offset], &node->key, key_size);
        current_offset += key_size;
        memcpy(
            &buffer.data[current_offset],
            &node->count_up,
            up_count_size
        );
        current_offset += up_count_size;
        memcpy(
            &buffer.data[current_offset],
            &node->count_down,
            down_count_size
        );
        current_offset += down_count_size;
        memcpy(
            &buffer.data[current_offset],
            &node->current_size,
            number_of_children_size
        );
        current_offset += number_of_children_size;
        buffer.size = current_offset;

        // write child keys and indices
        for (size_t j = 0; j < node->capacity; ++j) {
            const SequenceCountingTrie* child = node->map[j];
            if (child) {
                for (size_t k = 0; k < node_list.count; ++k) {
                    if (node_list.nodes[k] == child) {
                        ensure_buffer_capacity(
                            &buffer,
                            current_offset + child_entry_size
                        );
                        memcpy(
                            &buffer.data[current_offset],
                            &child->key,
                            key_size
                        );
                        current_offset += key_size;
                        memcpy(
                            &buffer.data[current_offset],
                            &k,
                            child_offset_size
                        );
                        current_offset += child_offset_size;
                        buffer.size = current_offset;
                        break;
                    }
                }
            }
        }
    }

    // replace the indices with global offsets
    current_offset = 0;
    for (size_t i = 0; i < node_list.count; ++i) {
        current_offset += number_of_children_offset;
        const uint64_t number_of_children =
            *(uint64_t*)(buffer.data + current_offset);
        current_offset += number_of_children_size;
        for (uint64_t j = 0; j < number_of_children; j++) {
            // Skip the child key
            current_offset += key_size;
            size_t child_index;
            memcpy(
                &child_index,
                &buffer.data[current_offset],
                child_offset_size
            );

            memcpy(
                &buffer.data[current_offset],
                &offsets[child_index],
                child_offset_size
            );
            current_offset += child_offset_size;
        }
    }
    fwrite(buffer.data, 1, buffer.size, output_file);
    fflush(output_file);
    free(buffer.data);
    free(node_list.nodes);
    free(node_list.depths);
    free(offsets);
}

/**
 * Reads the trie from the given file.
 *
 * @param filename The file to load the trie from.
 * @return The trie loaded from the file.
 */
FixedSizeTrie read_fixed_size_tree_file(const char* filename) {
    assert(filename != nullptr);
    const int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        exit(EXIT_FAILURE);
    }
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("Error getting file stats");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (st.st_size == 0) {
        perror("File is empty");
        close(fd);
        exit(EXIT_FAILURE);
    }

    const long int page_size = sysconf(_SC_PAGESIZE);
    const long int length = (st.st_size + page_size - 1) & ~(page_size - 1);
    char* trie_map = mmap(
        NULL,
        (size_t)length,
        PROT_READ,
        MAP_SHARED,
        fd,
        0
    );
    if (trie_map == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);

    const FixedSizeTrie trie = {
        .start = trie_map,
        .length = (size_t)length
    };

    return trie;
}

/**
 * Loads a trie for the given year.
 *
 * @param year The year of the trie to load.
 * @return The loaded trie.
 */
FixedSizeTrie load_tree_from_year(const uint16_t year) {
    // filePaths must be freed
    // -------------------------------------------------------------------------
    FilePathList file_data;
    get_all_files_paths_recursive(TREE_DATA_FOLDER, &file_data);
    if (file_data.file_paths == NULL) {
        perror("Could not retrieve file paths.\n");
        exit(EXIT_FAILURE);
    }

    const char* trie_file_path = nullptr;
    for (size_t i = 0; i < file_data.file_count; i++) {
        const char* file_path = file_data.file_paths[i];
        const char* lastSlashPosition = strrchr(file_path, '/');
        assert(lastSlashPosition != NULL);
        const char* file_name = lastSlashPosition + 1;
        char* end;

        // skip "tree_"
        file_name += 5;

        const uint16_t trie_year = (uint16_t)strtol(file_name, &end, 10);
        if (end == file_name) {
            perror("Failed to parse year");
            exit(EXIT_FAILURE);
        }
        if (trie_year == year) {
            trie_file_path = file_path;
            break;
        }
    }

    const FixedSizeTrie trie = read_fixed_size_tree_file(trie_file_path);

    // filePaths freed
    // -------------------------------------------------------------------------
    free_all_files_paths(&file_data);
    return trie;
}

/**
 * Gets the number of nodes in the given trie.
 *
 * @param trie The trie whose nodes to count.
 * @return The number of nodes in the given trie.
 */
uint64_t get_number_of_nodes(const char* trie) {
    const uint64_t number_of_children = get_number_of_children(trie);
    uint64_t number_of_nodes = 1;
    const char* start_offset = trie + child_list_offset;
    for (uint64_t child_index = 0;
         child_index < number_of_children;
         child_index++
    ) {
        const char* key_address =
            start_offset + (child_index * child_entry_size);
        const char* child_address = key_address + key_size;
        number_of_nodes +=
            get_number_of_nodes(trie + (*(const size_t*)child_address));
    }
    return number_of_nodes;
}

/**
 * Gets the maximum node size out of all the nodes in the given trie.
 *
 * @param trie The trie to find the max node size of.
 * @return The maximum node size of all the nodes in the given trie.
 */
uint64_t get_max_node_size(char* trie) {
    const uint64_t number_of_children = get_number_of_children(trie);
    uint64_t number_of_bytes =
        child_list_offset + (number_of_children * child_entry_size);
    const char* start_offset = trie + child_list_offset;
    for (uint64_t child_index = 0;
         child_index < number_of_children;
         child_index++
    ) {
        const char* key_address =
            start_offset + (child_index * child_entry_size);
        const char* child_address = key_address + key_size;
        const uint64_t child_number_of_bytes =
            get_max_node_size(trie + (*(const size_t*)child_address));
        if (child_number_of_bytes > number_of_bytes) {
            number_of_bytes = child_number_of_bytes;
        }
    }
    return number_of_bytes;
}

/**
 * Prints max node size and max number of nodes for all tries.
 */
void print_bounds_on_tries() {
    for (uint16_t year = START_YEAR; year < END_YEAR; year++) {
        // tree must be freed
        // ---------------------------------------------------------------------
        FixedSizeTrie trie = load_tree_from_year(year);
        printf(
            "Number of nodes for %hu: %lu\n",
            year,
            get_number_of_nodes(trie.start)
        );

        printf(
            "Max node size for %hu: %lu\n",
            year,
            get_max_node_size(trie.start)
        );

        // tree freed
        // ---------------------------------------------------------------------
        free_fixed_size_tree(&trie);
    }
}

/**
 * Frees the given trie.
 *
 * @param trie The trie to free.
 */
void free_fixed_size_tree(FixedSizeTrie* trie) {
    if (trie) {
        munmap(
            trie->start,
            trie->length
        );
        trie->start = NULL;
        trie->length = 0;
    }
}
