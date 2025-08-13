#include "tree_file_util_test.h"

#include <assert.h>

#include "../C/sequence_counting_trie.h"
#include "../C/tree_file_util.h"

void read_tree_matches_written_tree(void) {
    SequenceCountingTrie* root = create_sequence_counting_trie(-1);

    constexpr size_t window_size = 2;
    constexpr long sequence[] = {1, 2, 3};
    constexpr bool went_up[] = {true, false, true};
    constexpr bool went_up2[] = {false, true, false};

    add_sequence_to_trie(root, sequence, 3, went_up, window_size);
    add_sequence_to_trie(root, sequence, 3, went_up, window_size);
    add_sequence_to_trie(root, sequence, 3, went_up2, window_size);

    // Add a branch with a different first element
    constexpr long sequence2[] = {3, 2, 4};
    add_sequence_to_trie(root, sequence2, 3, went_up2, window_size);

    // Add a branch with a different last element
    constexpr long sequence3[] = {1, 4, 3};
    add_sequence_to_trie(root, sequence3, 3, went_up, window_size);

    const char* test_file_name = "tree_data/read_tree_matches_written_tree.test";
    FILE* f1 = fopen(test_file_name, "w+");
    assert(f1);
    export_tree_to_file(root, f1);
    FixedSizeTree read_tree_struct =
        read_fixed_size_tree_file(test_file_name);
    char* read_tree = read_tree_struct.start;

    char* node1 = get_child_with_key(read_tree, read_tree, 1);
    assert(get_down_count(node1) == 1);
    assert(get_up_count(node1)  == 3);

    char* node = get_child_with_key(read_tree, node1, 2);
    assert(get_down_count(node) == 2);
    assert(get_up_count(node) == 1);

    assert(get_child_with_key(read_tree, node, 3) == NULL);

    char* node3 = get_child_with_key(read_tree, read_tree, 3);
    assert(get_down_count(node3) == 1);
    assert(get_up_count(node3) == 0);

    node = get_child_with_key(read_tree, node3, 2);
    assert(get_down_count(node) == 0);
    assert(get_up_count(node) == 1);

    assert(get_child_with_key(read_tree, node, 4) == nullptr);

    node = get_child_with_key(read_tree, node1, 4);
    assert(get_down_count(node) == 1);
    assert(get_up_count(node) == 0);

    assert(get_child_with_key(read_tree, node, 3) == nullptr);

    free_trie(root);
    free_fixed_size_tree(&read_tree_struct);
}

void run_tree_file_util_test() {
    read_tree_matches_written_tree();
}
