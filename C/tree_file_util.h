#ifndef TREE_FILE_UTIL_H
#define TREE_FILE_UTIL_H

#include <stdio.h>

#include "sequence_counting_trie.h"

typedef struct {
    char* start;
    size_t length;
} FixedSizeTrie;

// Utility methods for acting on FixedSizeTree

size_t get_depth(const char* node);

long get_key(const char* node);

uint64_t get_up_count(const char* node);

uint64_t get_down_count(const char* node);

uint64_t get_number_of_children(const char* node);

char* get_child_with_key(char* root, const char* node, long child_key);

void get_prediction(
    const FixedSizeTrie* trie,
    const long* past_sequence,
    size_t sequence_length,
    double* prediction,
    size_t* depth
);

// Other methods

void save_yearly_tries();

void export_trie_to_file(const SequenceCountingTrie* trie, FILE* output_file);

FixedSizeTrie read_fixed_size_trie_from_file(const char* filename);

FixedSizeTrie load_trie_from_year(uint16_t year);

void print_bounds_on_tries();

void free_fixed_size_tree(FixedSizeTrie* trie);

#endif //TREE_FILE_UTIL_H
