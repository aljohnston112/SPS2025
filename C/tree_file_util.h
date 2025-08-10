#ifndef TREE_FILE_UTIL_H
#define TREE_FILE_UTIL_H
#include "tree.h"

TreeHashMap* load_trees_from_year(uint16_t year);

TreeHashMap* read_tree_csv(const char* filename);

void export_tree_to_csv(const TreeHashMap* root, FILE* out);

#endif //TREE_FILE_UTIL_H
