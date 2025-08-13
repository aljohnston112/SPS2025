#ifndef TREE_FILE_UTIL_H
#define TREE_FILE_UTIL_H

#include <stdio.h>

#include "sequence_counting_trie.h"

typedef struct {
    char* start;
    size_t length;
} FixedSizeTree;

size_t get_depth(const char* node);

long get_key(const char* node);

uint64_t get_up_count(const char* node);

uint64_t get_down_count(const char* node);

uint64_t get_number_of_children(const char* node);

char* get_child_with_key(char* root, const char* node, long child_key);

FixedSizeTree read_fixed_size_tree_file(const char* filename);

FixedSizeTree load_tree_from_year(uint16_t year);

void export_tree_to_file(const SequenceCountingTrie* root, FILE* out);

void free_fixed_size_tree(FixedSizeTree* tree);

void print_bounds_on_trees();

#endif //TREE_FILE_UTIL_H
