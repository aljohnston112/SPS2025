#include "tree_test.h"

#include "../C/tree.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>

void add_sequence_with_non_equal_sequence_creates_branches() {
    TreeHashMap* root = create_tree_hash_map(-1);

    constexpr size_t window_size = 2;
    constexpr long sequence[] = {1, 2, 3};
    constexpr bool went_up[] = {true, false, true};
    constexpr bool went_up2[] = {false, true, false};

    add_sequence(root, sequence, 3, went_up, window_size);
    add_sequence(root, sequence, 3, went_up, window_size);
    add_sequence(root, sequence, 3, went_up2, window_size);

    // Add a branch with a different first element
    constexpr long sequence2[] = {3, 2, 4};
    add_sequence(root, sequence2, 3, went_up2, window_size);

    // Add a branch with a different last element
    constexpr long sequence3[] = {1, 4, 3};
    add_sequence(root, sequence3, 3, went_up, window_size);

    const TreeHashMap* node1 = get_from_tree_hash_map(root, 1);
    assert(node1->count_down == 1);
    assert(node1->count_up == 3);

    const TreeHashMap* node = get_from_tree_hash_map(node1, 2);
    assert(node->count_down == 2);
    assert(node->count_up == 1);

    assert(get_from_tree_hash_map(node, 3) == NULL);

    const TreeHashMap* node3 = get_from_tree_hash_map(root, 3);
    assert(node3->count_down == 1);
    assert(node3->count_up == 0);

    node = get_from_tree_hash_map(node3, 2);
    assert(node->count_down == 0);
    assert(node->count_up == 1);

    assert(get_from_tree_hash_map(node, 4) == NULL);

    node = get_from_tree_hash_map(node1, 4);
    assert(node->count_down == 1);
    assert(node->count_up == 0);

    assert(get_from_tree_hash_map(node, 3) == NULL);

    free_tree(root);
}

void add_subsequence_with_same_sequence_added_three_times_sets_counts_correctly() {
    TreeHashMap* root = create_tree_hash_map(-1);

    constexpr long sequence[] = {1, 2};
    add_subsequence(root, sequence, 2, true);
    add_subsequence(root, sequence, 2, true);
    add_subsequence(root, sequence, 2, false);

    const TreeHashMap* node = get_from_tree_hash_map(root, 1);
    assert(node);
    assert(node->count_down == 0);
    assert(node->count_up == 0);

    node = get_from_tree_hash_map(node, 2);
    assert(node);
    assert(node->count_up == 2);
    assert(node->count_down == 1);

    free_tree(root);
}

void add_to_hash_map_or_get_with_collision_and_a_resize_contains_all_nodes_after_resize() {
    TreeHashMap* root = create_tree_hash_map(-1);

    TreeHashMap* node = add_to_hash_map_or_get(root, 1);
    assert(node != NULL && node->key == 1);
    assert(node->map);
    assert(node->size == START_MAP_SIZE);
    node->count_down = 1;
    node->count_up = -1;

    // Add a key that hashes to the same value
    const size_t initial_size = root->size;
    assert(root->size <= LONG_MAX);
    const long dup_key = (long)root->size + 1;
    node = add_to_hash_map_or_get(root, dup_key);
    assert(node != NULL && node->key == dup_key);
    assert(node->map);
    assert(node->size == START_MAP_SIZE);
    node->count_down = dup_key;
    node->count_up = -dup_key;

    assert(root->current_size == 2);

    // Trigger resize
    const size_t till_resize = root->size / 2;
    for (long i = 0; i < till_resize; i++) {
        node = add_to_hash_map_or_get(root, i + 2);
        assert(node != NULL && node->key == i + 2);
        assert(node->map);
        assert(node->size == START_MAP_SIZE);
        node->count_down = i + 2;
        node->count_up = -(i + 2);
    }

    assert(is_prime(root->size));
    assert(root->size > (2 * initial_size));
    assert(root->current_size == till_resize + 2);

    node = get_from_tree_hash_map(root, 1);
    assert(node != NULL && node->key == 1);
    assert(node->count_down == 1);
    assert(node->count_up == -1);
    assert(node->map);
    assert(node->size == START_MAP_SIZE);

    node = get_from_tree_hash_map(root, dup_key);
    assert(node != NULL && node->key == dup_key);
    assert(node->count_down == dup_key);
    assert(node->count_up == -dup_key);
    assert(node->map);
    assert(node->size == START_MAP_SIZE);

    for (long i = 0; i < till_resize; i++) {
        node = add_to_hash_map_or_get(root, i + 2);
        assert(node != NULL && node->key == i + 2);
        assert(node->count_down == i + 2);
        assert(node->count_up == -(i + 2));
        assert(node->map);
        assert(node->size == START_MAP_SIZE);
    }

    free_tree(root);
}

void get_from_tree_hash_map_on_non_inserted_key_with_collision_returns_null() {
    TreeHashMap* root = create_tree_hash_map(-1);

    assert(root->size <= LONG_MAX);
    const long dup_key = (long)root->size + 1;
    add_to_hash_map_or_get(root, 1);

    const TreeHashMap* result = get_from_tree_hash_map(root, dup_key);
    assert(result == NULL);

    free_tree(root);
}

void resize_hash_map_resizes_correctly() {

    TreeHashMap* root = create_tree_hash_map(-1);
    const size_t initial_size = root->size;
    TreeHashMap* child = add_to_hash_map_or_get(root, 10);
    child->count_down = 99;
    child->count_up = 100;
    resize_hash_map(root);

    assert(is_prime(root->size));
    assert(root->size > (2 * initial_size));
    const TreeHashMap* new_child = add_to_hash_map_or_get(root, 10);
    assert(new_child == child);
    assert(new_child->count_down == 99);
    assert(new_child->count_up == 100);
    assert(new_child->current_size == 0);
    assert(new_child->map != NULL);
    assert(new_child->size == child->size);

    free_tree(root);
}

void create_tree_hash_map_initializes_tree_correctly() {
    TreeHashMap* root = create_tree_hash_map(-1);
    assert(root != NULL);
    assert(root->key == -1);
    assert(root->current_size == 0);
    assert(root->count_up == 0);
    assert(root->count_down == 0);
    assert(root->size == START_MAP_SIZE);

    for (size_t i = 0; i < START_MAP_SIZE; i++) {
        assert(root->map[i] == NULL);
    }

    free_tree(root);
}

void run_tree_test() {
    create_tree_hash_map_initializes_tree_correctly();
    resize_hash_map_resizes_correctly();
    get_from_tree_hash_map_on_non_inserted_key_with_collision_returns_null();
    add_to_hash_map_or_get_with_collision_and_a_resize_contains_all_nodes_after_resize();
    add_subsequence_with_same_sequence_added_three_times_sets_counts_correctly();
    add_sequence_with_non_equal_sequence_creates_branches();
}