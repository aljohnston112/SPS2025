#ifndef TREE_H
#define TREE_H

#define MAX_INDEX 2048
#include <sys/types.h>

typedef struct TreeNode TreeNode;

struct TreeNode {
    TreeNode* children[MAX_INDEX];
    ulong counts[MAX_INDEX];
    bool is_loss;
};

TreeNode* create_node(bool is_loss);

void free_tree(TreeNode* node);

TreeNode* find_child(const TreeNode* parent, long key);

void add_child(TreeNode* parent, long key, TreeNode* child);

void add_sequence(
    TreeNode* parent,
    const long* sequence,
    size_t sequence_length
);

void print_tree(const TreeNode* root);

ulong count_empty_slots(const TreeNode* node, ulong* total_slots);

#endif //TREE_H