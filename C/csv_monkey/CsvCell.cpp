#include <cstdlib>
#include "CsvCell.h"
#include <cerrno>
#include <climits>
#include <cstring>
#include <charconv>


extern "C" {
/**
 * The caller is responsible for deallocating the returned string using `free()`.
 * errno may be set to ENOMEM if there is no memory left.
 * @param self
 * @return A null-terminated string that must be deallocated when no longer needed.
 */
char* csv_cell_as_str(const CsvCell* self) {
    // +1 for null
    const auto str = static_cast<char*>(malloc(self->size + 1));
    if (str == nullptr) {
        return nullptr;
    }
    memcpy(str, self->ptr, self->size);
    str[self->size] = '\0';
    return str;
}

/**
 * @param self
 * @param out_value A pointer to the parsed int.
 * @return 0 if the parse was successful
 *        ERANGE if the integer was out of range
 *        EINVAL if no conversion was performed
 */
int csv_cell_as_int(const CsvCell* self, int* out_value) {
    char* endptr;
    const long value = strtol(self->ptr, &endptr, 10);
    if (errno != 0) {
        return -errno;
    }
    if (value > INT_MAX || value < INT_MIN) {
        return ERANGE;
    }
    if (static_cast<size_t>(endptr - self->ptr) != self->size) {
        return EINVAL;
    }
    *out_value = static_cast<int>(value);
    return errno;
}

/**
 *
 * @param self
* @param out_value A pointer to the parsed double.
 * @return 0 if the parse was successful
 *        ERANGE if the integer was out of range
 *        EINVAL if no conversion was performed
 */
int csv_cell_as_double(const CsvCell* self, double* out_value) {
    std::from_chars(
        self->ptr,
        self->ptr + self->size,
        *out_value
    );
    return 0;
}
}
