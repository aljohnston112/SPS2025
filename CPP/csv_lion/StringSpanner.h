#ifndef STRINGSPANNER_H
#define STRINGSPANNER_H

#ifndef __SSE4_2__
    #include <string.h>
    #define CHARSET_SIZE 256
#else
#include <immintrin.h>
#endif

typedef struct StringSpanner {
    // The character vector
#ifndef __SSE4_2__
    unsigned char charset[CHARSET_SIZE];
#else
    __m128i v_;
#endif
} StringSpanner;

#endif //STRINGSPANNER_H

void string_spanner_init(
    StringSpanner* self,
    char c1, char c2, char c3, char c4
);

int string_spanner_operator(
    const StringSpanner* self,
    char* buf
);