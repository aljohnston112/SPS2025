#include "tree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

LinkedNode* create_linked_node(const long key) {
    LinkedNode* node = malloc(sizeof(LinkedNode));
    if (node == NULL) {
        perror("malloc failed");
        exit(-1);
    }

    node->next = NULL;
    node->key = key;
    node->value = create_tree_node();
    node->count = 1;
    return node;
}

void free_linked_list(LinkedNode* node) {
    if (!node) return;

    // Load the linked lists into previous
    // since we can't free a predecessor before its successor
    ulong count = 0;
    LinkedNode* previous[MAX_INDEX];
    LinkedNode* current_node = node;
    while (current_node->next) {
        previous[count++] = current_node;
        current_node = current_node->next;
    }

    // Free from the back to the front
    for (ulong i = count - 1; i != 0; i--) {
        free_tree(previous[i]->value);
        free(previous[i]);
    }
    free_tree(previous[0]->value);
    free(previous[0]);
}

LinkedNode* add_to_linked_list(LinkedNode** head, const long key) {
    if (*head == NULL) {
        (*head) = create_linked_node(key);
        return (*head);
    }

    if (key < (*head)->key) {
        LinkedNode* new_node = create_linked_node(key);
        new_node->next = *head;
        *head = new_node;
        return new_node;
    }

    LinkedNode* prev_node = *head;
    LinkedNode* current_node = *head;
    while (current_node != NULL && current_node->key < key) {
        prev_node = current_node;
        current_node = current_node->next;
    }
    if (current_node != NULL && current_node->key == key) {
        current_node->count++;
        return current_node;
    }

    LinkedNode* new_node = create_linked_node(key);
    if (current_node == NULL) {
        prev_node->next = new_node;
    } else {
        new_node->next = current_node->next;
        current_node->next = new_node;
    }
    return new_node;
}

TreeNode* create_tree_node() {
    TreeNode* node = malloc(sizeof(TreeNode));
    if (node == NULL) {
        perror("malloc failed");
        exit(-1);
    }
    node->children = NULL;
    return node;
}

void free_tree(TreeNode* node) {
    if (!node) return;
    free_linked_list(node->children);
}

TreeNode* find_child_in_tree_node(const TreeNode* parent, const long key) {
    assert(key < MAX_INDEX && key > -MAX_INDEX);
    
    const LinkedNode* current_node = parent->children;
    if (current_node == NULL) {
        return NULL;
    }
    if (current_node->key == key) {
        return current_node->value;
    }

    while (current_node->next) {
        current_node = current_node->next;
        if (current_node == NULL) {
            return NULL;
        }
        if (current_node->key == key) {
            return current_node->value;
        }
    }

    return NULL;
}

TreeNode* add_child_to_tree(TreeNode* parent, const long key) {
    assert(key < MAX_INDEX && key > -MAX_INDEX);
    if (parent->children == NULL) {
        parent->children = create_linked_node(key);
        return parent->children->value;
    }
    return add_to_linked_list(&parent->children, key)->value;
}

void add_subsequence(
    TreeNode* parent,
    const long* sequence,
    const size_t sequence_length
) {
    TreeNode* current = parent;
    for (size_t i = 0; i < sequence_length; i++) {
        const long key = sequence[i];
        current = add_child_to_tree(current, key);
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
                TreeNode* child = find_child_in_tree_node(parent, first);
                if (child != NULL) {
                    add_subsequence(
                        child,
                        sequence + i,
                        j - i
                    );
                } else {
                    add_subsequence(
                        parent->children->value,
                        sequence + i,
                        j - i
                    );

                }
            }
        }
    }
}

void print_tree_helper(const TreeNode* node, const int current_depth) {
    if (node == NULL) return;

    if (current_depth < 5) {
        const LinkedNode* current = node->children;
        while (current != NULL) {
            if (current->count > 0) {
                printf(
                    "Depth %d: %ld (%s: %lu)\n",
                    current_depth,
                    current->key,
                    current->key < 0 ? "loss" : "profit",
                    current->count
                );
                if (current->value != NULL) {
                    print_tree_helper(current->value, current_depth + 1);
                }
            }
            current = current->next;
        }
    }
}

void print_tree(const TreeNode* root) {
    if (root == NULL) {
        printf("(empty tree)\n");
        return;
    }

    printf("Tree structure:\n");
    print_tree_helper(root, 1);
}
