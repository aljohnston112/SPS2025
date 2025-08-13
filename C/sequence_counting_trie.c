#include "sequence_counting_trie.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Creates the root of a sequence counting tree.
 * The returned tree must be freed by the caller.
 *
 * @param key The key of the tree's root node.
 *
 * @return The root of the tree.
 */
SequenceCountingTrie* create_sequence_counting_trie(const long key) {
    SequenceCountingTrie* trie = malloc(sizeof(SequenceCountingTrie));
    if (trie == NULL) {
        fprintf(stderr, "malloc failed");
        exit(EXIT_FAILURE);
    }
    trie->map = calloc(START_MAP_SIZE, sizeof(SequenceCountingTrie*));
    if (trie->map == NULL) {
        fprintf(stderr, "malloc failed");
        exit(EXIT_FAILURE);
    }
    trie->capacity = START_MAP_SIZE;
    trie->current_size = 0;
    trie->key = key;
    trie->count_down = 0;
    trie->count_up = 0;
    return trie;
}

/**
 * Gets the child whose root contains the given key from the given tree.
 * If the child does not exist, it will be added as a child of the given tree.
 *
 * @param trie The tree to search for the child.
 * @param key The key of the child to search for.
 * @return The child with the given key.
 */
SequenceCountingTrie* add_to_trie_or_get(SequenceCountingTrie* trie,
                                         const long key) {
    uint64_t map_size = trie->capacity;
    if (__glibc_unlikely(trie->current_size * 2 >= map_size)) {
        // printf("Resize #: %ld\n", ++resizes);
        resize_trie_map(trie);
        map_size = trie->capacity;
    }

    SequenceCountingTrie* child = get_from_trie(trie, key);
    if (child != NULL) {
        return child;
    }

    size_t i = 0;
    // find unoccupied spot
    while (trie->map[((size_t)key + (i * i)) % map_size] != NULL) {
        i++;
    }
    // put a new child in the map
    const size_t index = ((size_t)key + (i * i)) % map_size;
    child = create_sequence_counting_trie(key);
    trie->map[index] = child;
    trie->current_size++;
    return child;
}

/**
 * Ensures the given key sequence exists in the trie and updates its count.
 *
 * Traverses the trie following the given sequence of keys, creating
 * nodes as needed. At the node corresponding to the last key in the
 * sequence, either the "count_up" or "count_down" counter are incremented.
 *
 * @param parent_trie The parent to start the search from.
 *                    When crawling the trie, the parent key is ignored.
 * @param key_sequence The sequence of keys.
 * @param sequence_length The length of the sequence.
 * @param went_up If true, the child's up count will be incremented,
 *                else, the down count will be incremented.
 */
void add_subsequence_to_trie(
    SequenceCountingTrie* parent_trie,
    const long* key_sequence,
    const size_t sequence_length,
    const bool went_up
) {
    // trace out the path through the tree, adding children as necessary
    SequenceCountingTrie* current_trie = parent_trie;
    for (size_t i = 0; i < sequence_length; i++) {
        const long key = key_sequence[i];
        SequenceCountingTrie* child = add_to_trie_or_get(current_trie, key);
        current_trie = child;
    }

    // increment the correct count
    const uint64_t past_count_up = current_trie->count_up;
    const uint64_t past_count_down = current_trie->count_down;
    if (went_up) {
        current_trie->count_up++;
    } else {
        current_trie->count_down++;
    }
    if (__glibc_unlikely(current_trie->count_down < past_count_down)) {
        fprintf(stderr, "Overflow");
        exit(EXIT_FAILURE);
    }
    if (__glibc_unlikely(current_trie->count_up < past_count_up)) {
        fprintf(stderr, "Overflow");
        exit(EXIT_FAILURE);
    }
}

/**
 * Ensures the given key sequence exists in the trie
 * and updates the count of each node corresponding to each key.
 *
 * @param trie The trie to add the sequence to.
 * @param sequence The sequence to add.
 * @param sequence_length The length of the sequence.
 * @param went_up An array of whether the stock went up in the future.
 *                Each index should correspond to the given key
 *                with the same index.
 * @param window_size The max depth to stop adding keys at.
 *                    If the sequence is longer,
 *                    then only the first window_size keys will be added.
 */
void add_sequence_to_trie(
    SequenceCountingTrie* trie,
    const long* sequence,
    const size_t sequence_length,
    const bool* went_up,
    const size_t window_size
) {
    size_t length = sequence_length;
    if (length > window_size) {
        length = window_size;
    }
    for (size_t i = 0; i < length; i++) {
        add_subsequence_to_trie(
            trie,
            sequence,
            length - i,
            went_up[length - i - 1]
        );
    }
}

/**
 * Gets the child trie of the given trie that has the given key.
 *
 * @param trie The trie to get the child from.
 * @param key The key of the child trie.
 * @return The child trie with the given key, or null if it does not exist.
 */
SequenceCountingTrie* get_from_trie(
    const SequenceCountingTrie* trie,
    const long key
) {
    assert(trie);
    SequenceCountingTrie** children = trie->map;
    const size_t number_of_children = trie->capacity;
    if (children == NULL) {
        return nullptr;
    }
    assert(number_of_children > 0);

    size_t i = 0;
    while (i < number_of_children) {
        const size_t index = ((size_t)key + (i * i)) % number_of_children;
        SequenceCountingTrie* child = children[index];
        if (child == NULL) {
            return nullptr;
        }
        if (child->key == key) {
            return child;
        }
        i++;
    }
    return nullptr;
}

void get_prediction_from_trie_helper(
    const SequenceCountingTrie* trie,
    const long* past_sequence,
    const size_t sequence_length,
    double* prediction,
    size_t* depth
) {
    assert(prediction != NULL);

    const SequenceCountingTrie* current_trie = trie;
    for (size_t i = 0; i < sequence_length; i++) {
        current_trie = get_from_trie(current_trie, past_sequence[i]);
        if (current_trie == NULL) {
            *prediction = 0.0;
            return;
        }
        (*depth)++;
    }

    const uint64_t count_up = current_trie->count_up;
    const uint64_t count_down = current_trie->count_down;
    const uint64_t total_count = count_down + count_up;
    if (count_up > count_down) {
        *prediction = (double)count_up / (double)total_count;
    } else {
        *prediction = -((double)count_down / (double)total_count);
    }
}

/**
 * Gets the probability that the given sequence will result in profit
 * in the future.
 *
 * @param trie The trie to crawl.
 * @param past_sequence The key sequence to use when crawling the trie.
 * @param sequence_length The length of the given ket sequence.
 * @param prediction Will hold the prediction on return.
 *                   Will be 0 if the key sequence is not in the tree,
 *                   a negative number if the prediction is loss of profit,
 *                   else a positive number.
 * @param depth Will hold the depth at which the prediction was found at.
 */
void get_prediction_from_trie(
    const SequenceCountingTrie* trie,
    const long* past_sequence,
    const size_t sequence_length,
    double* prediction,
    size_t* depth
) {
    for (size_t i = sequence_length; i > 0; i--) {
        size_t current_depth = 0;
        double current_prediction = 0.0;
        get_prediction_from_trie_helper(
            trie,
            past_sequence + (sequence_length - i),
            i,
            &current_prediction,
            &current_depth
        );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        if (current_prediction != 0.0) {
#pragma GCC diagnostic pop
            *prediction = current_prediction;
            *depth = current_depth;
            break;
        }
    }
}

/**
 * Resizes the trie's child map.
 *
 * @param trie The trie whose child map will be resized.
 */
void resize_trie_map(SequenceCountingTrie* trie) {
    assert(trie);
    assert(trie->map);
    assert(trie->capacity > 0);

    // Get the first prime number over twice as large as the current size
    size_t newSize = 2 * trie->capacity + 1;
    while (!is_prime(newSize)) {
        newSize += 2;
    }
    const size_t old_size = trie->capacity;
    SequenceCountingTrie** old_map = trie->map;
    trie->map = calloc(newSize, sizeof(SequenceCountingTrie*));
    if (trie->map == NULL) {
        fprintf(stderr, "calloc failed");
        exit(EXIT_FAILURE);
    }
    trie->capacity = newSize;
    trie->current_size = 0;
    for (size_t i = 0; i < old_size; i++) {
        if (old_map[i] != NULL) {
            SequenceCountingTrie* old_child = old_map[i];
            size_t j = 0;
            // slot is occupied
            while (
                trie->map[((size_t)old_child->key + (j * j)) % trie->capacity]
                !=
                NULL
            ) {
                j++;
            }
            // found a spot to put the key, put it in the map
            const size_t index =
                ((size_t)old_child->key + (j * j)) % trie->capacity;
            trie->map[index] = old_child;
            trie->current_size++;
        }
    }

    free(old_map);
}

void print_trie_helper(
    const SequenceCountingTrie* node,
    const int current_depth
) {
    constexpr int min_depth = 1;
    constexpr int max_depth = 10000; // min_depth + 1;
    if (node == NULL || current_depth > max_depth) {
        return;
    }

    size_t i = 0;
    while (i < node->capacity) {
        const SequenceCountingTrie* child = node->map[i];
        if (child != NULL) {
            if (current_depth >= min_depth
                // &&
                // child->count_down < child->count_up &&
                // child->count_up > 9 &&
                // (double)child->count_up / (double)child->count_down > 2
            ) {
                printf(
                    "Depth %d: %ld (Up: %lu)(Down: %lu)\n",
                    current_depth,
                    child->key,
                    child->count_up,
                    child->count_down
                );
            }
            print_trie_helper(child, current_depth + 1);
        }
        i++;
    }
}

/**
 * Prints the given trie to stdout.
 * @param trie The given tree to print.
 */
void print_trie(const SequenceCountingTrie* trie) {
    if (trie == NULL) {
        printf("(empty tree)\n");
        return;
    }

    printf("Tree structure:\n");
    print_trie_helper(trie, 1);
}


void free_trie_helper(SequenceCountingTrie* node) {
    if (node == NULL) {
        return;
    }

    size_t i = 0;
    while (i < node->capacity) {
        SequenceCountingTrie* child = node->map[i];
        if (child != NULL) {
            free_trie_helper(child);
        }
        i++;
    }
    free(node->map);
    free(node);
}

/**
 * Frees the allocated memory of the trie.
 * This frees every single map in the entire tree.
 *
 * @param trie The trie to free.
 */
void free_trie(SequenceCountingTrie* trie) {
    if (trie == NULL) {
        return;
    }
    free_trie_helper(trie);
}

/**
 * Checks if a number is prime.
 *
 * @param value The value to check the primeness of.
 * @return True if the value is prime, else false.
 */
bool is_prime(const size_t value) {
    assert(value > 0);
    size_t i = 0;
    while (primes[i] != 0 && value > primes[i]) {
        i++;
    }
    return value == primes[i];
}
