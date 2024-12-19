#include "StringSpanner.h"

#ifndef __SSE4_2__
inline void string_spanner_init(
    StringSpanner* self,
    const char c1, const char c2, const char c3, const char c4
) {
    memset(self->charset, 0, sizeof(self->charset));

    self->charset[(unsigned char)c1] = 1;
    self->charset[(unsigned char)c2] = 1;
    self->charset[(unsigned char)c3] = 1;
    self->charset[(unsigned char)c4] = 1;
    self->charset[0] = 1; // Null terminator
}

/**
 * @param self
 * @param buf The string to search for delimiters.
 * @return The index of the next character that matches
 *         any of the four characters in StringSpannerFallback's charset.
 *         Only the first 16 characters in string will be searched,
 *         and 16 will be returned if no match was found.
 */
inline size_t string_spanner_operator(
    const StringSpanner* self,
    const char* buf
) {
    const char* ptr = buf;
    const char* endPtr = ptr + 16;

    bool found = false;

    ptr -= 4;
    while (!found && ptr < endPtr) {
        ptr += 4;
        if (self->charset[(unsigned char)(ptr[0])]) {
            found = 1;
        } else if (self->charset[(unsigned char)(ptr[1])]) {
            ptr++;
            found = 1;
        } else if (self->charset[(unsigned char)(ptr[2])]) {
            ptr += 2;
            found = 1;
        } else if (self->charset[(unsigned char)(ptr[3])]) {
            ptr += 3;
            found = 1;
        }
    }

    if (!*ptr) {
        return 16; // Report NULL encountered as no match
    }

    return ptr - buf;
}
#else
void string_spanner_init(
    StringSpanner* self,
    const char c1, const char c2, const char c3, const char c4
) {
    self->v_ = _mm_set_epi8(
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        c4, c3, c2, c1
    );
}

size_t string_spanner_operator(
    const StringSpanner* self,
    const char* buf
) {
    return _mm_cmpistri(
        self->v_,
        _mm_loadu_si128((__m128i *)(buf)),
        _SIDD_UBYTE_OPS |
        _SIDD_CMP_EQUAL_ANY |
        _SIDD_POSITIVE_POLARITY
    );
}
#endif