#include "tree.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/**
 *
 * @param map
 * @param past_sequence The sequence must start with a positive number
 * @param sequence_length
 * @return -1 if the sequence was not found
 *            or there is no sequence in the tree
 *            with a length 1 greater than the given sequence length.
 *            Otherwise, a predicted number of days the next sequence will last.
 */
void get_prediction_from_hash_map_helper(
    const TreeHashMap* map,
    const long* past_sequence,
    const size_t sequence_length,
    double* prediction
) {
    assert(prediction != NULL);

    const TreeHashMap* current_map = map;
    for (size_t i = 0; i < sequence_length; i++) {
        assert(past_sequence[i] != 0);
        current_map = get_from_tree_hash_map(current_map, past_sequence[i]);
        if (current_map == NULL) {
            *prediction = 0.0;
            return;
        }
    }

    // TODO add total count to each node if tests are promising
    u_int64_t total_count = 0;
    for (size_t i = 0; i < current_map->size; i++) {
        const TreeHashMap* child = current_map->map[i];
        if (child != NULL) {
            const u_int64_t old_total_count = total_count;
            total_count += child->count;
            if (__glibc_unlikely(total_count <= old_total_count)) {
                perror("Overflow");
                exit(1);
            }
        }
    }

    *prediction = 0.0;
    for (size_t i = 0; i < current_map->size; i++) {
        const TreeHashMap* child = current_map->map[i];
        if (child != NULL) {
            const double p = (double)child->key * ((double)child->count / (
                double)total_count);
            *prediction += p;
        }
    }
}

void get_prediction_from_hash_map(
    const TreeHashMap* map,
    const long* past_sequence,
    const size_t sequence_length,
    double* prediction
) {
    for (size_t i = 0; i < sequence_length; i += 2) {
        double p;
        get_prediction_from_hash_map_helper(
            map,
            past_sequence + i,
            sequence_length - i,
            &p
        );
        if (p != 0.0) {
            *prediction = p;
        }
    }
}


TreeHashMap* create_tree_hash_map(const long key) {
    TreeHashMap* map = malloc(sizeof(TreeHashMap));
    if (map == NULL) {
        perror("malloc failed");
        exit(-1);
    }
    map->map = calloc(sizeof(TreeHashMap*), START_MAP_SIZE);
    if (map->map == NULL) {
        perror("malloc failed");
        exit(-1);
    }
    map->size = START_MAP_SIZE;
    map->current_size = 0;
    map->key = key;
    map->count = 0;
    return map;
}

bool isPrime(const size_t value) {
    assert(value > 0);
    size_t i = 0;
    while (primes[i] != 0 && value > primes[i]) {
        i++;
    }
    return value == primes[i];
}

size_t getFirstPrimeEqualOrLess(const size_t n) {
    size_t low = 0;
    size_t high = sizeof(primes) / sizeof(primes[0]) - 1;
    while (low <= high) {
        const size_t mid = low + (high - low) / 2;
        if (primes[mid] == n) {
            return n;
        }
        if (primes[mid] < n) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return primes[high];
}

TreeHashMap* get_from_tree_hash_map(const TreeHashMap* map, const long key) {
    assert(map);
    if (map->map == NULL) {
        return NULL;
    }
    assert(map->size > 0);

    size_t hash_multiplier = 0;
    const long actual_key = key < 0 ? -key : key;
    while (true) {
        const size_t i =
            (actual_key + (hash_multiplier * hash_multiplier)) % map->size;
        if (map->map[i] == NULL) {
            break;
        }
        if (map->map[i]->key == key) {
            return map->map[i];
        }
        hash_multiplier++;
    }
    return NULL;
}

void resize_hash_map(TreeHashMap* map);

u_int64_t resizes = 0;

TreeHashMap* add_to_hash_map(TreeHashMap* map, const long key) {
    u_int64_t map_size = map->size;
    if (__glibc_unlikely(map->current_size * 2 >= map_size)) {
        printf("Resize #: %ld\n", ++resizes);
        resize_hash_map(map);
        map_size = map->size;
    }

    TreeHashMap* child = get_from_tree_hash_map(map, key);
    if (child != NULL) {
        return child;
    }

    size_t i = 0;
    const long actual_key = key < 0 ? -key : key;
    assert(key != 0);
    // slot is occupied
    while (map->map[(actual_key + (i * i)) % map_size] != NULL) {
        i++;
    }
    // found a spot to put the key, put it in the map
    const size_t index = (actual_key + (i * i)) % map_size;

    child = create_tree_hash_map(key);
    map->map[index] = child;
    map->current_size++;
    return child;
}

void resize_hash_map(TreeHashMap* map) {
    assert(map);
    assert(map->map);
    assert(map->size > 0);

    // Get the first prime number over twice as large as the current size
    size_t newSize = 2 * map->size + 1;
    while (!isPrime(newSize)) {
        newSize += 2;
    }
    const size_t old_size = map->size;
    const size_t old_count = map->count;
    TreeHashMap** old_map = map->map;
    map->map = calloc(sizeof(TreeHashMap*), newSize);
    map->size = newSize;
    map->current_size = 0;
    map->count = old_count;

    const size_t prime = getFirstPrimeEqualOrLess(map->size);
    for (int i = 0; i < old_size; i++) {
        if (old_map[i] != NULL) {
            TreeHashMap* value = old_map[i];
            int j = 0;
            const long actual_key = value->key < 0 ? -value->key : value->key;
            const size_t secondaryHashFunction = prime - (actual_key % prime);
            // slot is occupied
            while (map->map[(actual_key + j * secondaryHashFunction) % map->
                size] != NULL) {
                j++;
            }
            // found a spot to put the key, put it in the map
            const size_t index = (actual_key + j * secondaryHashFunction) % map
                ->size;

            map->map[index] = value;
            map->current_size++;
        }
    }

    free(old_map);
}

void add_subsequence(
    TreeHashMap* parent,
    const long* sequence,
    const size_t sequence_length
) {
    TreeHashMap* current = parent;
    for (size_t i = 0; i < sequence_length; i++) {
        const long key = sequence[i];
        TreeHashMap* child = add_to_hash_map(current, key);
        current = child;
    }
    current->count++;
    if (__glibc_unlikely(current->count == 0)) {
        perror("Overflow");
        exit(1);
    }
}

void add_sequence(
    TreeHashMap* parent,
    const long* sequence,
    const size_t sequence_length
) {
    for (size_t i = 0; i < sequence_length; i++) {
        for (size_t j = i + 1; j <= sequence_length; j++) {
            const long first = *(sequence + i);
            if (first > 0) {
                add_subsequence(
                    parent,
                    sequence + i,
                    j - i
                );
            } else {
                TreeHashMap* child = get_from_tree_hash_map(parent, *(sequence + i - i));
                if (child != NULL) {
                    add_subsequence(
                        child,
                        sequence + i,
                        j - i
                    );
                } else {
                    add_subsequence(
                        add_to_hash_map(parent, *(sequence + i - i)),
                        sequence + i,
                        j - i
                    );
                }
            }
        }
    }
}

void print_tree_helper(const TreeHashMap* node, const int current_depth) {
    if (node == NULL) return;

    if (current_depth < 40) {
        size_t i = 0;
        while (i < node->size) {
            const TreeHashMap* child = node->map[i];
            if (child != NULL) {
                printf(
                    "Depth %d: %ld (%s: %lu)\n",
                    current_depth,
                    child->key,
                    current_depth % 2 == 0 ? "loss" : "profit",
                    child->count
                );
                print_tree_helper(child, current_depth + 1);
            }
            i++;
        }
    }
}

void print_tree(const TreeHashMap* root) {
    if (root == NULL) {
        printf("(empty tree)\n");
        return;
    }

    printf("Tree structure:\n");
    print_tree_helper(root, 1);
}