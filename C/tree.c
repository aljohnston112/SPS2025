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
    const HashMap* map,
    const long* past_sequence,
    const size_t sequence_length,
    double* prediction
) {
    assert(prediction != NULL);

    const HashMap* current_map = map;
    for (size_t i = 0; i < sequence_length; i++) {
        assert(past_sequence[i] != 0);
        current_map = get_from_hash_map(current_map, past_sequence[i]);
        if (current_map == NULL) {
            *prediction = 0.0;
            return;
        }
    }

    // TODO add total count to each node if tests are promising
    u_int64_t total_count = 0;
    for (size_t i = 0; i < current_map->size; i++) {
        const HashMap* child = current_map->map[i];
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
        const HashMap* child = current_map->map[i];
        if (child != NULL) {
            const double p = (double)child->key * ((double)child->count / (
                double)total_count);
            *prediction += p;
        }
    }
}

void get_prediction_from_hash_map(
    const HashMap* map,
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


HashMap* create_hash_map(const long key) {
    HashMap* map = malloc(sizeof(HashMap));
    if (map == NULL) {
        perror("malloc failed");
        exit(-1);
    }
    map->map = calloc(sizeof(HashMap*), START_MAP_SIZE);
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

HashMap* get_from_hash_map(const HashMap* map, const long key) {
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

void resize_hash_map(HashMap* map);

u_int64_t resizes = 0;

HashMap* add_to_hash_map(HashMap* map, const long key) {
    u_int64_t map_size = map->size;
    if (__glibc_unlikely(map->current_size * 2 >= map_size)) {
        printf("Resize #: %ld\n", ++resizes);
        resize_hash_map(map);
        map_size = map->size;
    }

    HashMap* child = get_from_hash_map(map, key);
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

    child = create_hash_map(key);
    map->map[index] = child;
    map->current_size++;
    return child;
}

void resize_hash_map(HashMap* map) {
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
    HashMap** old_map = map->map;
    map->map = calloc(sizeof(HashMap*), newSize);
    map->size = newSize;
    map->current_size = 0;
    map->count = old_count;

    const size_t prime = getFirstPrimeEqualOrLess(map->size);
    for (int i = 0; i < old_size; i++) {
        if (old_map[i] != NULL) {
            HashMap* value = old_map[i];
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
    HashMap* parent,
    const long* sequence,
    const size_t sequence_length
) {
    HashMap* current = parent;
    for (size_t i = 0; i < sequence_length; i++) {
        const long key = sequence[i];
        HashMap* child = add_to_hash_map(current, key);
        current = child;
    }
    current->count++;
    if (__glibc_unlikely(current->count == 0)) {
        perror("Overflow");
        exit(1);
    }
}

void add_sequence(
    HashMap* parent,
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
                HashMap* child = get_from_hash_map(parent, *(sequence + i - i));
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

void print_tree_helper(const HashMap* node, const int current_depth) {
    if (node == NULL) return;

    if (current_depth < 40) {
        size_t i = 0;
        while (i < node->size) {
            const HashMap* child = node->map[i];
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

void print_tree(const HashMap* root) {
    if (root == NULL) {
        printf("(empty tree)\n");
        return;
    }

    printf("Tree structure:\n");
    print_tree_helper(root, 1);
}

// TreeNode* create_tree_node() {
//     TreeNode* node = malloc(sizeof(TreeNode));
//     if (node == NULL) {
//         perror("malloc failed");
//         exit(-1);
//     }
//     node->children = NULL;
//     return node;
// }
//
// void free_tree(TreeNode* node) {
//     if (!node) return;
//     free_linked_list(node->children);
// }

// TreeNode* find_child_in_tree_node(const TreeNode* parent, const long key) {
//     assert(key < MAX_INDEX && key > -MAX_INDEX);
//
//     const TreeLinkedNode* current_node = parent->children;
//     if (current_node == NULL) {
//         return NULL;
//     }
//     if (current_node->key == key) {
//         return current_node->value;
//     }
//
//     while (current_node->next) {
//         current_node = current_node->next;
//         if (current_node == NULL) {
//             return NULL;
//         }
//         if (current_node->key == key) {
//             return current_node->value;
//         }
//     }
//
//     return NULL;
// }

// TreeNode* add_child_to_tree(TreeNode* parent, const long key) {
//     assert(key < MAX_INDEX && key > -MAX_INDEX);
//     if (parent->children == NULL) {
//         parent->children = create_linked_node(key);
//         return parent->children->value;
//     }
//     return add_to_linked_list(&parent->children, key)->value;
// }

// void print_tree_helper(const TreeNode* node, const int current_depth) {
//     if (node == NULL) return;
//
//     if (current_depth < 3) {
//         const TreeLinkedNode* current = node->children;
//         while (current != NULL) {
//             if (current->count > 0) {
//                 printf(
//                     "Depth %d: %ld (%s: %lu)\n",
//                     current_depth,
//                     current->key,
//                     current_depth % 2 == 0 ? "loss" : "profit",
//                     current->count
//                 );
//                 if (current->value != NULL) {
//                     print_tree_helper(current->value, current_depth + 1);
//                 }
//             }
//             current = current->next;
//         }
//     }
// }
//
// void print_tree(const TreeNode* root) {
//     if (root == NULL) {
//         printf("(empty tree)\n");
//         return;
//     }
//
//     printf("Tree structure:\n");
//     print_tree_helper(root, 1);
// }

/** The linked list implementation used this, but is no longer needed **/

// TreeLinkedNode* create_linked_node(const long key) {
//     TreeLinkedNode* node = malloc(sizeof(TreeLinkedNode));
//     if (node == NULL) {
//         perror("malloc failed");
//         exit(-1);
//     }
//
//     node->next = NULL;
//     node->key = key;
//     node->value = create_tree_node();
//     node->count = 1;
//     return node;
// }
//
// void free_linked_list(TreeLinkedNode* node) {
//     if (!node) return;
//
//     // Load the linked lists into previous
//     // since we can't free a predecessor before its successor
//     u_int64_t count = 0;
//     TreeLinkedNode* previous[MAX_INDEX];
//     TreeLinkedNode* current_node = node;
//     while (current_node->next) {
//         previous[count++] = current_node;
//         current_node = current_node->next;
//     }
//
//     // Free from the back to the front
//     for (u_int64_t i = count - 1; i != 0; i--) {
//         free_tree(previous[i]->value);
//         free(previous[i]);
//     }
//     free_tree(previous[0]->value);
//     free(previous[0]);
// }
//
// TreeLinkedNode* add_to_linked_list(TreeLinkedNode** head, const long key) {
//     if (*head == NULL) {
//         (*head) = create_linked_node(key);
//         return (*head);
//     }
//
//     if (key < (*head)->key) {
//         TreeLinkedNode* new_node = create_linked_node(key);
//         new_node->next = *head;
//         *head = new_node;
//         return new_node;
//     }
//
//     TreeLinkedNode* prev_node = *head;
//     TreeLinkedNode* current_node = *head;
//     while (current_node != NULL && current_node->key < key) {
//         prev_node = current_node;
//         current_node = current_node->next;
//     }
//
//     if (current_node != NULL && current_node->key == key) {
//         current_node->count++;
//         if (current_node->count == 0) {
//             perror("overflow detected");
//             exit(-1);
//         }
//         return current_node;
//     }
//
//     if (current_node == NULL) {
//         TreeLinkedNode* new_node = create_linked_node(key);
//         prev_node->next = new_node;
//         return new_node;
//     }
//
//     if (current_node->key == key) {
//         current_node->count++;
//         if (current_node->count == 0) {
//             perror("overflow detected");
//             exit(-1);
//         }
//         return current_node;
//     }
//
//     TreeLinkedNode* new_node = create_linked_node(key);
//     new_node->next = current_node->next;
//     current_node->next = new_node;
//     prev_node->next = new_node;
//     return new_node;
// }
