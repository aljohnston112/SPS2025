#ifndef SEQUENCE_ANALYZER_H
#define SEQUENCE_ANALYZER_H

#include "ranks.h"
#include "tree.h"

typedef struct SequenceNode {
    int64_t* sequence;
    size_t length;
    struct SequenceNode* next;
} SequenceNode;

typedef struct {
    int64_t* seq;
    size_t seq_len;
    char** symbols;
    bool* went_up;
    size_t n_symbols;
    size_t capacity;
} SeqMatches;

typedef struct {
    SeqMatches* seq_matches;
    size_t n_seq_matches;
    size_t capacity;
} SeqMatchList;

void analyze_all_sequences_fill_matches(
    const TreeHashMap* root,
    const SymbolToRanksHashMap* all_stocks,
    SeqMatchList* sequences
);


#endif //SEQUENCE_ANALYZER_H