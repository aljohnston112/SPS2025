#include "tree.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "ranks.h"

void get_prediction_from_hash_map_helper(
    const TreeHashMap* map,
    const long* past_sequence,
    const size_t sequence_length,
    double* prediction,
    size_t* depth
) {
    assert(prediction != NULL);

    const TreeHashMap* current_map = map;
    for (size_t i = 0; i < sequence_length; i++) {
        current_map = get_from_tree_hash_map(current_map, past_sequence[i]);
        if (current_map == NULL) {
            *prediction = 0.0;
            return;
        }
        (*depth)++;
    }

    const u_int64_t count_up = current_map->count_up;
    const u_int64_t count_down = current_map->count_down;
    const u_int64_t total_count = count_down + count_up;
    if (count_up > count_down) {
        *prediction = (double)count_up / (double)total_count;
    } else {
        *prediction = -((double)count_down / (double)total_count);
    }
}

void get_prediction_from_hash_map(
    const TreeHashMap* map,
    const long* past_sequence,
    const size_t sequence_length,
    double* prediction,
    size_t* depth
) {
    for (size_t i = sequence_length; i > 0; i--) {
        size_t d = 0;
        double p;
        get_prediction_from_hash_map_helper(
            map,
            past_sequence + (sequence_length - i),
            i,
            &p,
            &d
        );
        if (p != 0.0) {
            *prediction = p;
            *depth = d;
            break;
        }
    }
}

void print_tree_helper(const TreeHashMap* node, const int current_depth) {
    constexpr int min_depth = 1;
    constexpr int max_depth = 10000; // min_depth + 1;
    if (node == NULL || current_depth > max_depth) return;

    size_t i = 0;
    while (i < node->size) {
        const TreeHashMap* child = node->map[i];
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
            print_tree_helper(child, current_depth + 1);
        }
        i++;
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

typedef struct {
    const TreeHashMap* node;
    int row_index;
    int depth;
} QueueItem;

void export_tree_to_csv(const TreeHashMap* root, FILE* out) {
    if (!root || !out) return;

    int front = 0;
    int back = 0;
    int next_row = 0;

    constexpr int MAX_NODES = 10000;
    QueueItem** queue = malloc(MAX_NODES * sizeof(QueueItem*));
    queue[back] = malloc(sizeof(QueueItem));
    queue[back]->node = root;
    queue[back]->row_index = next_row;
    queue[back]->depth = 0;
    back++;

    const TreeHashMap** node_index_map =
        malloc(MAX_NODES * sizeof(TreeHashMap*));
    node_index_map[next_row++] = root;

    // Precalculate max children across all nodes
    size_t max_children = 0;
    while (front < back) {
        assert(front < MAX_NODES);
        const TreeHashMap* node = queue[front++]->node;
        size_t child_count = 0;
        for (size_t i = 0; i < node->size; ++i) {
            if (node->map[i]) {
                ++child_count;
            }
        }
        if (child_count > max_children) {
            max_children = child_count;
        }

        for (size_t i = 0; i < node->size; ++i) {
            const TreeHashMap* child = node->map[i];
            if (child) {
                queue[back] = malloc(sizeof(QueueItem));
                queue[back]->node = child;
                queue[back]->row_index = next_row;
                queue[back]->depth = queue[front - 1]->depth + 1;
                back++;
                node_index_map[next_row++] = child;
            }
        }
    }

    // Write CSV header
    fprintf(out, "Depth,Key,Up,Down");

    for (size_t i = 0; i < max_children; ++i) {
        fprintf(out, ",C%zu", i);
    }
    fprintf(out, "\n");

    // Output all rows in order
    for (int i = 0; i < next_row; ++i) {
        const TreeHashMap* node = node_index_map[i];
        const int depth = queue[i]->depth;

        fprintf(
            out,
            "%d,%ld,%lu,%lu",
            depth,
            node->key,
            node->count_up,
            node->count_down
        );

        // Write child indices
        size_t written = 0;
        for (size_t j = 0; j < node->size; ++j) {
            const TreeHashMap* child = node->map[j];
            if (child) {
                for (int k = 0; k < next_row; ++k) {
                    if (node_index_map[k] == child) {
                        fprintf(out, ",%d", k + 2);
                        break;
                    }
                }
                written++;
            }
        }
        while (written++ < max_children) {
            fprintf(out, ",-1");
        }

        fprintf(out, "\n");
    }

    for (size_t i = 0; i < MAX_NODES; ++i) {
        if (queue[i]) {
            free(queue[i]);
        }
    }
    free(queue);
    free(node_index_map);
}

void free_tree_helper(TreeHashMap* node) {
    if (node == NULL) {
        return;
    }

    size_t i = 0;
    while (i < node->size) {
        TreeHashMap* child = node->map[i];
        if (child != NULL) {
            free_tree_helper(child);
        }
        i++;
    }
    free(node->map);
    free(node);
}

void free_tree(TreeHashMap* root) {
    if (root == NULL) {
        return;
    }
    free_tree_helper(root);
}




TreeHashMap* get_from_tree_hash_map(const TreeHashMap* map, const long key) {
    assert(map);
    TreeHashMap** children = map->map;
    const size_t number_of_children = map->size;
    if (children == NULL) {
        return nullptr;
    }
    assert(number_of_children > 0);

    size_t i = 0;
    while (true) {
        const size_t index = (key + (i * i)) % number_of_children;
        TreeHashMap* child = children[index];
        if (child == NULL) {
            return nullptr;
        }
        if (child->key == key) {
            return child;
        }
        i++;
    }
}

bool is_prime(const size_t value) {
    assert(value > 0);
    size_t i = 0;
    while (primes[i] != 0 && value > primes[i]) {
        i++;
    }
    return value == primes[i];
}

void resize_hash_map(TreeHashMap* map) {
    assert(map);
    assert(map->map);
    assert(map->size > 0);

    // Get the first prime number over twice as large as the current size
    size_t newSize = 2 * map->size + 1;
    while (!is_prime(newSize)) {
        newSize += 2;
    }
    const size_t old_size = map->size;
    TreeHashMap** old_map = map->map;
    map->map = calloc(newSize, sizeof(TreeHashMap*));
    map->size = newSize;
    map->current_size = 0;
    for (size_t i = 0; i < old_size; i++) {
        if (old_map[i] != NULL) {
            TreeHashMap* old_child = old_map[i];
            int j = 0;
            // slot is occupied
            while (map->map[(old_child->key + (j * j)) % map->size] != NULL) {
                j++;
            }
            // found a spot to put the key, put it in the map
            const size_t index = (old_child->key + (j * j)) % map->size;

            map->map[index] = old_child;
            map->current_size++;
        }
    }

    free(old_map);
}

TreeHashMap* add_to_hash_map_or_get(TreeHashMap* map, const long key) {
    u_int64_t map_size = map->size;
    if (__glibc_unlikely(map->current_size * 2 >= map_size)) {
        // printf("Resize #: %ld\n", ++resizes);
        resize_hash_map(map);
        map_size = map->size;
    }

    TreeHashMap* child = get_from_tree_hash_map(map, key);
    if (child != NULL) {
        return child;
    }

    size_t i = 0;
    // slot is occupied
    while (map->map[(key + (i * i)) % map_size] != NULL) {
        i++;
    }
    // found a spot to put the key, put it in the map
    const size_t index = (key + (i * i)) % map_size;

    child = create_tree_hash_map(key);
    map->map[index] = child;
    map->current_size++;
    return child;
}

void add_subsequence(
    TreeHashMap* parent,
    const long* sequence,
    const size_t sequence_length,
    const bool went_up
) {
    TreeHashMap* current = parent;
    for (size_t i = 0; i < sequence_length; i++) {
        const long key = sequence[i];
        TreeHashMap* child = add_to_hash_map_or_get(current, key);
        current = child;
    }

    const uint64_t past_count_up = current->count_up;
    const uint64_t past_count_down = current->count_down;
    if (went_up) {
        current->count_up++;
    } else {
        current->count_down++;
    }
    if (__glibc_unlikely(current->count_down < past_count_down)) {
        perror("Overflow");
        exit(1);
    }
    if (__glibc_unlikely(current->count_up < past_count_up)) {
        perror("Overflow");
        exit(1);
    }
}

void add_sequence(
    TreeHashMap* root,
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
        add_subsequence(
            root,
            sequence,
            length - i,
            went_up[length - i - 1]
        );
    }
}

/**
 * Creates the root of a sequence counting tree.
 * The returned tree must be freed by the caller.
 *
 * @param key The key of the root.
 *            This key is skipped over when sequences are added.
 *            This key is also skipped over when making predictions.
 *
 * @return The root of the tree.
 */
TreeHashMap* create_tree_hash_map(const long key) {
    TreeHashMap* map = malloc(sizeof(TreeHashMap));
    if (map == NULL) {
        perror("malloc failed");
        exit(-1);
    }
    map->map = calloc(START_MAP_SIZE, sizeof(TreeHashMap*));
    if (map->map == NULL) {
        perror("malloc failed");
        exit(-1);
    }
    map->size = START_MAP_SIZE;
    map->current_size = 0;
    map->key = key;
    map->count_down = 0;
    map->count_up = 0;
    return map;
}
