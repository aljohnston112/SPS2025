#include <stdio.h>
#include <stdlib.h>

#include "internal_config.h"
#include "tree_file_util.h"

/**
 *        depth: size_t,
 *        key: long,
 *        up_count: uint64_t,
 *        down_count: uint64_t,
 *        number_of_children: uint64_t,
 *        A number_of_children sized list of these:
 *            child_key: long
 *            child_offset: size_t
 *
 */
int main(int argc, char* argv[]) {
    char* filename;
    asprintf(
        &filename,
        "%s/%s",
        TREE_DATA_FOLDER,
        "tree_1966_1_5_12.tree"
    );

    if (filename == nullptr) {
        perror("Failed to creat file name");
        return -1;
    }

    FixedSizeTrie trie = read_fixed_size_trie_from_file(filename);
    free(filename);
    char* current_node = trie.start;
    
    const uint64_t number_of_children = get_number_of_children(current_node);
    fprintf(
        stdout,
        "depth: %zu\n"
        "key: %ld\n"
        "up count: %lu\n"
        "down count: %lu\n"
        "number_of_children: %lu\n\n",
        get_depth(current_node),
        get_key(current_node),
        get_down_count(current_node),
        get_up_count(current_node),
        number_of_children
    );
    const TrieChildKeyAddressPair* children = get_child_keys(current_node);
    for (size_t i = 0; i < number_of_children; i++) {
        fprintf(
            stdout,
            "address: %zu\n"
            "key: %ld\n",
            children[i].address,
            children[i].key
        );
    }

    free_fixed_size_tree(&trie);
}
