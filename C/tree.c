#include "tree.h"

#include <stdio.h>
#include <stdlib.h>

TreeNode* create_node(const bool is_loss) {
    TreeNode* node = malloc(sizeof(TreeNode));
    if (node == NULL) {
        perror("malloc failed");
        exit(-1);
    }
    node->is_loss = is_loss;
    for (int i = 0; i < MAX_INDEX; i++) {
        node->children[i] = nullptr;
        node->counts[i] = 0;
    }
    return node;
}

void free_tree(TreeNode* node) {
    if (!node) return;
    for (int i = 0; i < MAX_INDEX; i++) {
        if (node->children[i]) {
            free_tree(node->children[i]);
        }
    }
    free(node);
}

TreeNode* find_child(const TreeNode* parent, long key) {
    if (key >= MAX_INDEX || key <= -MAX_INDEX) return nullptr;
    if (key < 0) {
        key = -key;
    }
    return parent->children[key];
}

void add_child(TreeNode* parent, long key, TreeNode* child) {
    if (key >= MAX_INDEX || key <= -MAX_INDEX) {
        fprintf(stderr, "Key %lu is too large\n", key);
        exit(-1);
    }
    if (key < 0) {
        key = -key;
    }
    parent->children[key] = child;
    parent->counts[key]++;
}

void add_subsequence(
    TreeNode* parent,
    const long* sequence,
    const size_t sequence_length
) {
    TreeNode* current = parent;
    for (size_t i = 0; i < sequence_length; i++) {
        const long key = sequence[i];
        TreeNode* child = find_child(current, key);
        if (!child) {
            child = create_node(key < 0);
            add_child(current, key, child);
        } else {
            if (key > 0) {
                current->counts[key]++;
            } else {
                current->counts[-key]++;
            }
        }
        current = child;
    }
}

void add_sequence(
    TreeNode* parent,
    const long* sequence,
    const size_t sequence_length
) {
    for (size_t i = 0; i < sequence_length; i++) {
        for (size_t j = i + 1; j <= sequence_length; j++) {
            const long first = (sequence + i)[0];
            if (first > 0) {
                add_subsequence(
                    parent,
                    sequence + i,
                    j - i
                );
            } else {
                if (parent->children[-first]) {
                    add_subsequence(
                        parent->children[-first],
                        sequence + i,
                        j - i
                    );
                } else {
                    TreeNode* child = create_node(first < 0);
                    add_child(parent, first, child);
                    add_subsequence(
                        child,
                        sequence + i,
                        j - i
                    );
                }
            }
        }
    }
}

void print_tree_helper(const struct TreeNode* node, int current_depth,
                       long parent_key) {
    if (node == NULL) return;

    if (current_depth < 3) {
        for (long i = 1; i < MAX_INDEX; i++) {
            if (node->counts[i] > 0) {
                printf("Depth %d: %ld (%s: %lu)\n",
                       current_depth,
                       i,
                       node->is_loss ? "loss" : "profit",
                       node->counts[i]);

                if (node->children[i] != NULL) {
                    print_tree_helper(node->children[i], current_depth + 1, i);
                }
            }
        }
    }
}

void print_tree(const struct TreeNode* root) {
    if (root == NULL) {
        printf("(empty tree)\n");
        return;
    }

    printf("Tree structure:\n");
    print_tree_helper(root, 1, 0);
}

ulong count_empty_slots(const TreeNode* node, ulong* total_slots) {
    ulong empty_count = 0;
    bool hasChildren = false;
    for (int i = 0; i < MAX_INDEX; i++) {
        if (node->children[i] == NULL) {
            empty_count++;
        } else {
            empty_count += count_empty_slots(node->children[i], total_slots);
            hasChildren = true;
        }
        (*total_slots)++;
    }
    if (!hasChildren) {
        *total_slots -= MAX_INDEX;
        empty_count -= MAX_INDEX;
    }
    return empty_count;
}