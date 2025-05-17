#ifndef TREE_H
#define TREE_H

#define MAX_INDEX 2048
#include <sys/types.h>

typedef struct TreeNode TreeNode;
typedef struct LinkedNode LinkedNode;

struct LinkedNode {
    LinkedNode* next;
    long key;
    TreeNode* value;
    ulong count;
};

struct TreeNode {
    LinkedNode* children;
};

LinkedNode* create_linked_node(long key);
void free_linked_list(LinkedNode* node);

LinkedNode* add_to_linked_list(LinkedNode** head, long key);

TreeNode* create_tree_node();

void free_tree(TreeNode* node);

TreeNode* find_child_in_tree_node(const TreeNode* parent, long key);

TreeNode* add_child_to_tree(TreeNode* parent, long key);

void add_sequence(
    TreeNode* parent,
    const long* sequence,
    size_t sequence_length
);

void print_tree(const TreeNode* root);

#endif //TREE_H