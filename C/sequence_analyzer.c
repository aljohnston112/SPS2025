#include "sequence_analyzer.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void init_seq_matches(SeqMatches* sm, int64_t* seq, const size_t seq_len) {
    sm->seq = seq;
    sm->seq_len = seq_len;
    sm->capacity = 8;
    sm->n_symbols = 0;
    sm->symbols = malloc(sm->capacity * sizeof(char*));
    sm->went_up = malloc(sm->capacity * sizeof(bool));
}

void free_seq_matches(const SeqMatches* sm) {
    free(sm->symbols);
    free(sm->went_up);
    // do NOT free sm->seq here, it’s managed elsewhere
}

void free_seq_match_list(const SeqMatchList* sml) {
    for (size_t i = 0; i < sml->n_seq_matches; i++) {
        for (size_t j = 0; j < sml->seq_matches[i].n_symbols; j++) {
            free(sml->seq_matches[i].symbols[j]);
        }
        free_seq_matches(&sml->seq_matches[i]);
    }
    free(sml->seq_matches);
}

void add_seq_match(SeqMatchList* sml, int64_t* seq, const size_t seq_len) {
    if (sml->n_seq_matches == sml->capacity) {
        sml->capacity *= 2;
        sml->seq_matches = realloc(
            sml->seq_matches,
            sml->capacity * sizeof(SeqMatches)
        );
        if (!sml->seq_matches) {
            exit(EXIT_FAILURE);
        }
    }
    init_seq_matches(&sml->seq_matches[sml->n_seq_matches], seq, seq_len);
    sml->n_seq_matches++;
}

void add_match(SeqMatches* sm, char* symbol, const bool went_up) {
    if (sm->n_symbols == sm->capacity) {
        sm->capacity *= 2;
        sm->symbols = realloc(sm->symbols, sm->capacity * sizeof(char*));
        sm->went_up = realloc(sm->went_up, sm->capacity * sizeof(bool));
    }
    sm->symbols[sm->n_symbols] = symbol;
    sm->went_up[sm->n_symbols] = went_up;
    sm->n_symbols++;
}

void init_seq_match_list(SeqMatchList* sml) {
    sml->capacity = 8;
    sml->n_seq_matches = 0;
    sml->seq_matches = malloc(sml->capacity * sizeof(SeqMatches));
}


bool match_sequence(
    const int64_t* rank_per_day,
    const int64_t* seq,
    const size_t seq_len
) {
    for (size_t i = 0; i < seq_len; i++) {
        if (rank_per_day[i] != seq[i]) {
            return false;
        }
    }
    return true;
}

void collect_profitable_sequences_helper(
    const TreeHashMap* node,
    int64_t* current_seq,
    const size_t depth,
    SeqMatchList* results
) {
    assert(current_seq);
    if (node == nullptr) return;

    if (depth > 0 &&
        node->count_up > 9 &&
        node->count_up > 2 * node->count_down
    ) {
        // Copy current sequence for storage
        int64_t* seq_copy = malloc(depth * sizeof(int64_t));
        memcpy(seq_copy, current_seq, depth * sizeof(int64_t));
        add_seq_match(results, seq_copy, depth);
    }

    for (size_t i = 0; i < node->size; i++) {
        const TreeHashMap* child = node->map[i];
        if (child != nullptr) {
            current_seq[depth] = child->key;
            collect_profitable_sequences_helper(
                child,
                current_seq,
                depth + 1,
                results
            );
        }
    }
}

void collect_profitable_sequences(
    const TreeHashMap* root,
    SeqMatchList* results
) {
    init_seq_match_list(results);

    int64_t* current_seq = malloc(MAX_TREE_DEPTH * sizeof(int64_t));
    collect_profitable_sequences_helper(root, current_seq, 0, results);
    free(current_seq);
}

void find_sequence_in_all_stocks_fill_matches(
    SeqMatches* sm,
    const SymbolToRanksHashMap* all_stocks
) {
    for (size_t s = 0; s < all_stocks->count; s++) {
        const StockRanks* stock_ranks =
            all_stocks->symbol_to_ranks[s];
        if (!stock_ranks) {
            continue;
        }
        for (size_t i = DAYS_PER_DIFF;
             i + sm->seq_len < stock_ranks->data_size;
             i++
        ) {
            if (match_sequence(
                    stock_ranks->rank_per_day + i,
                    sm->seq,
                    sm->seq_len
                )
            ) {
                add_match(
                    sm,
                    stock_ranks->stock_symbol,
                    stock_ranks->went_up[i]
                );
            }
        }
    }
}

void analyze_all_sequences_fill_matches(
    const TreeHashMap* root,
    const SymbolToRanksHashMap* all_stocks,
    SeqMatchList* sequences
) {
    collect_profitable_sequences(root, sequences);
    for (size_t i = 0; i < sequences->n_seq_matches; i++) {
        find_sequence_in_all_stocks_fill_matches(
            &sequences->seq_matches[i],
            all_stocks
        );
    }
}