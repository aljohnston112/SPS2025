#include "sequence_counting_trie_test.h"

#include "../C/sequence_counting_trie.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

void add_sequence_with_non_equal_sequence_creates_branches() {
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

    const SequenceCountingTrie* node1 = get_from_trie(root, 1);
    assert(node1->count_down == 1);
    assert(node1->count_up == 3);

    const SequenceCountingTrie* node = get_from_trie(node1, 2);
    assert(node->count_down == 2);
    assert(node->count_up == 1);

    assert(get_from_trie(node, 3) == NULL);

    const SequenceCountingTrie* node3 = get_from_trie(root, 3);
    assert(node3->count_down == 1);
    assert(node3->count_up == 0);

    node = get_from_trie(node3, 2);
    assert(node->count_down == 0);
    assert(node->count_up == 1);

    assert(get_from_trie(node, 4) == NULL);

    node = get_from_trie(node1, 4);
    assert(node->count_down == 1);
    assert(node->count_up == 0);

    assert(get_from_trie(node, 3) == NULL);

    free_trie(root);
}

void
add_subsequence_with_same_sequence_added_three_times_sets_counts_correctly() {
    SequenceCountingTrie* root = create_sequence_counting_trie(-1);

    constexpr long sequence[] = {1, 2};
    add_subsequence_to_trie(root, sequence, 2, true);
    add_subsequence_to_trie(root, sequence, 2, true);
    add_subsequence_to_trie(root, sequence, 2, false);

    const SequenceCountingTrie* node = get_from_trie(root, 1);
    assert(node);
    assert(node->count_down == 0);
    assert(node->count_up == 0);

    node = get_from_trie(node, 2);
    assert(node);
    assert(node->count_up == 2);
    assert(node->count_down == 1);

    free_trie(root);
}

void
add_to_hash_map_or_get_with_collision_and_a_resize_contains_all_nodes_after_resize() {
    SequenceCountingTrie* root = create_sequence_counting_trie(-1);

    SequenceCountingTrie* node = add_to_trie_or_get(root, 1);
    assert(node != NULL && node->key == 1);
    assert(node->map);
    assert(node->capacity == START_MAP_SIZE);
    node->count_down = 1;
    node->count_up = 2;

    // Add a key that hashes to the same value
    const size_t initial_size = root->capacity;
    assert(root->capacity <= LONG_MAX);
    const long dup_key = (long)root->capacity + 1;
    node = add_to_trie_or_get(root, dup_key);
    assert(node != NULL && node->key == dup_key);
    assert(node->map);
    assert(node->capacity == START_MAP_SIZE);
    node->count_down = dup_key;
    node->count_up = dup_key * dup_key;

    assert(root->current_size == 2);

    // Trigger resize
    const uint64_t till_resize = root->capacity / 2;
    for (uint64_t i = 0; i < till_resize; i++) {
        node = add_to_trie_or_get(root, i + 2);
        assert(node != NULL && node->key == i + 2);
        assert(node->map);
        assert(node->capacity == START_MAP_SIZE);
        node->count_down = i + 2;
        node->count_up = (i + 2) * (i + 2);
    }

    assert(is_prime(root->capacity));
    assert(root->capacity > (2 * initial_size));
    assert(root->current_size == till_resize + 2);

    node = get_from_trie(root, 1);
    assert(node != NULL && node->key == 1);
    assert(node->count_down == 1);
    assert(node->count_up == 2);
    assert(node->map);
    assert(node->capacity == START_MAP_SIZE);

    node = get_from_trie(root, dup_key);
    assert(node != NULL && node->key == dup_key);
    assert(node->count_down == (uint64_t)dup_key);
    assert(dup_key * dup_key > dup_key);
    assert(node->count_up == (uint64_t)(dup_key * dup_key));
    assert(node->map);
    assert(node->capacity == START_MAP_SIZE);

    for (uint32_t i = 0; i < till_resize; i++) {
        node = add_to_trie_or_get(root, i + 2);
        assert(node != NULL && node->key == i + 2);
        assert(node->count_down == (uint64_t)(i + 2));
        assert((i + 2) * (i + 2) > (i + 2));
        assert(node->count_up == (uint64_t)((i + 2) * (i + 2)));
        assert(node->map);
        assert(node->capacity == START_MAP_SIZE);
    }

    free_trie(root);
}

void get_from_tree_hash_map_on_non_inserted_key_with_collision_returns_null() {
    SequenceCountingTrie* root = create_sequence_counting_trie(-1);

    assert(root->capacity <= LONG_MAX);
    const long dup_key = (long)root->capacity + 1;
    add_to_trie_or_get(root, 1);

    const SequenceCountingTrie* result = get_from_trie(root, dup_key);
    assert(result == NULL);

    free_trie(root);
}

void resize_hash_map_resizes_correctly() {
    SequenceCountingTrie* root = create_sequence_counting_trie(-1);
    const size_t initial_size = root->capacity;
    SequenceCountingTrie* child = add_to_trie_or_get(root, 10);
    child->count_down = 99;
    child->count_up = 100;
    resize_trie_map(root);

    assert(is_prime(root->capacity));
    assert(root->capacity > (2 * initial_size));
    const SequenceCountingTrie* new_child = add_to_trie_or_get(root, 10);
    assert(new_child == child);
    assert(new_child->count_down == 99);
    assert(new_child->count_up == 100);
    assert(new_child->current_size == 0);
    assert(new_child->map != NULL);
    assert(new_child->capacity == child->capacity);

    free_trie(root);
}

void create_sequence_counting_trie_initializes_tree_correctly() {
    SequenceCountingTrie* root = create_sequence_counting_trie(-1);
    assert(root != NULL);
    assert(root->key == -1);
    assert(root->current_size == 0);
    assert(root->count_up == 0);
    assert(root->count_down == 0);
    assert(root->capacity == START_MAP_SIZE);

    for (size_t i = 0; i < START_MAP_SIZE; i++) {
        assert(root->map[i] == NULL);
    }

    free_trie(root);
}

void fill_trie_fills_trie_correctly() {
    auto stock_rank0 = (StockRanks){
        .stock_symbol = "a",
        .rank_diffs = (int64_t[]){0, 1, 0, 1, 2, 0, 1, 2, 3},
        .went_up = (bool[]){
            false, true, false, true, false, false, false, false, true
        },
        .capacity = 9
    };

    auto stock_rank1 = (StockRanks){
        .stock_symbol = "b",
        .rank_diffs = (int64_t[]){0, 1, 0},
        .went_up = (bool[]){false, true, false},
        .capacity = 3
    };
    SymbolToRanksHashMap* symbol_to_ranks_map =
        calloc(1, sizeof(SymbolToRanksHashMap));
    add_to_rank_hash_map(symbol_to_ranks_map, &stock_rank0);
    add_to_rank_hash_map(symbol_to_ranks_map, &stock_rank1);

    SequenceCountingTrie* trie = create_sequence_counting_trie(-1);
    fill_trie(symbol_to_ranks_map, trie, 1, 2, 3);

    const SequenceCountingTrie* child0 = get_from_trie(trie, 0);
    assert(child0->count_down == 2);
    assert(child0->count_up == 0);
    assert(child0->current_size == 1);
    assert(child0->key == 0);

    const SequenceCountingTrie* child01 = get_from_trie(child0, 1);
    assert(child01->count_down == 0);
    assert(child01->count_up == 1);
    assert(child01->current_size == 1);
    assert(child01->key == 1);

    const SequenceCountingTrie* child012 = get_from_trie(child01, 2);
    assert(child012->count_down == 1);
    assert(child012->count_up == 0);
    assert(child012->current_size == 0);
    assert(child012->key == 2);

    auto child1 = get_from_trie(trie, 1);
    assert(child1->count_down == 0);
    assert(child1->count_up == 2);
    assert(child1->current_size == 2);
    assert(child1->key == 1);

    const SequenceCountingTrie* child10 = get_from_trie(child1, 0);
    assert(child10->count_down == 1);
    assert(child10->count_up == 0);
    assert(child10->current_size == 1);
    assert(child10->key == 0);

    const SequenceCountingTrie* child101 = get_from_trie(child10, 1);
    assert(child101->count_down == 0);
    assert(child101->count_up == 1);
    assert(child101->current_size == 0);
    assert(child101->key == 1);

    const SequenceCountingTrie* child12 = get_from_trie(child1, 2);
    assert(child12->count_down == 1);
    assert(child12->count_up == 0);
    assert(child12->current_size == 1);
    assert(child12->key == 2);

    const SequenceCountingTrie* child120 = get_from_trie(child12, 0);
    assert(child120->count_down == 1);
    assert(child120->count_up == 0);
    assert(child120->current_size == 0);
    assert(child120->key == 0);

    const SequenceCountingTrie* child2 = get_from_trie(trie, 2);
    assert(child2->count_down == 1);
    assert(child2->count_up == 0);
    assert(child2->current_size == 1);
    assert(child2->key == 2);

    const SequenceCountingTrie* child20 = get_from_trie(child2, 0);
    assert(child20->count_down == 1);
    assert(child20->count_up == 0);
    assert(child20->current_size == 0);
    assert(child20->key == 0);

    const SequenceCountingTrie* child201 = get_from_trie(child20, 1);
    assert(child201 == nullptr);

    free(symbol_to_ranks_map);
}

void get_prediction_from_trie_returns_correct_predictions() {
    auto stock_rank0 = (StockRanks){
        .stock_symbol = "a",
        .rank_diffs = (int64_t[]){0, 1, 0, 1, 2, 0, 1, 2, 3},
        .went_up = (bool[]){
            false, true, false, true, false, false, false, false, true
        },
        .capacity = 9
    };
    SymbolToRanksHashMap* symbol_to_ranks_map =
        calloc(1, sizeof(SymbolToRanksHashMap));
    add_to_rank_hash_map(symbol_to_ranks_map, &stock_rank0);

    SequenceCountingTrie* trie = create_sequence_counting_trie(-1);
    fill_trie(symbol_to_ranks_map, trie, 1, 2, 3);

    double prediction = 0;
    size_t depth = 0;
    get_prediction_from_trie(
        trie,
        (const long[]){1},
        1,
        &prediction,
        &depth
    );
    assert(prediction == 1);

    get_prediction_from_trie(
        trie,
        (const long[]){1, 2},
        2,
        &prediction,
        &depth
    );
    assert(prediction == -1);

    get_prediction_from_trie(
        trie,
        (const long[]){1, 2, 0},
        3,
        &prediction,
        &depth
    );
    assert(prediction == -1);

    get_prediction_from_trie(
        trie,
        (const long[]){1, 0},
        2,
        &prediction,
        &depth
    );
    assert(prediction == -1);

    get_prediction_from_trie(
        trie,
        (const long[]){1, 0, 1},
        3,
        &prediction,
        &depth
    );
    assert(prediction == 1);

    get_prediction_from_trie(
        trie,
        (const long[]){0},
        1,
        &prediction,
        &depth
    );
    assert(prediction == -1);

    get_prediction_from_trie(
        trie,
        (const long[]){0, 1},
        2,
        &prediction,
        &depth
    );
    assert(prediction == 1);

    get_prediction_from_trie(
        trie,
        (const long[]){0, 1, 2},
        3,
        &prediction,
        &depth
    );
    assert(prediction == -1);

    get_prediction_from_trie(
        trie,
        (const long[]){2},
        1,
        &prediction,
        &depth
    );
    assert(prediction == -1);

    get_prediction_from_trie(
        trie,
        (const long[]){2, 0},
        2,
        &prediction,
        &depth
    );
    assert(prediction == -1);

    printf("");
}

void run_tree_test() {
    create_sequence_counting_trie_initializes_tree_correctly();
    resize_hash_map_resizes_correctly();
    get_from_tree_hash_map_on_non_inserted_key_with_collision_returns_null();
    add_to_hash_map_or_get_with_collision_and_a_resize_contains_all_nodes_after_resize();
    add_subsequence_with_same_sequence_added_three_times_sets_counts_correctly();
    add_sequence_with_non_equal_sequence_creates_branches();
    fill_trie_fills_trie_correctly();
    get_prediction_from_trie_returns_correct_predictions();
}
