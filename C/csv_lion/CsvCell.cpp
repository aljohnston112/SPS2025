#include <cstdlib>
#include "CsvCell.h"
#include <cerrno>
#include <cstring>
#include <charconv>
#include <cstdio>

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
        perror("malloc failed");
        return nullptr;
    }
    memcpy(str, self->ptr, self->size);
    str[self->size] = '\0';
    return str;
}

/**
 *
 * @param self
 * @param out_value A pointer to the parsed double.
 * @return 0 if the parse was successful
 *         ERANGE if the integer was out of range
 *         EINVAL if no conversion was performed
 */
int csv_cell_as_double(const CsvCell* self, double* out_value) {
    const auto [_, ec] = std::from_chars(
        self->ptr,
        self->ptr + self->size,
        *out_value
    );
    if (ec == std::errc::invalid_argument) {
        return EINVAL;
    }
    if (ec == std::errc::result_out_of_range) {
        return ERANGE;
    }
    return 0;
}

/**
*
* @param first
* @param last
* @param out_value A pointer to the parsed double.
* @return 0 if the parse was successful
*         ERANGE if the integer was out of range
*         EINVAL if no conversion was performed
*/
int extract_uint8_t(const char* first, const char* last, u_int8_t* out_value) {
    const auto [_, ec] = std::from_chars(
        first,
        last,
        *out_value
    );
    if (ec == std::errc::invalid_argument) {
        return EINVAL;
    }
    if (ec == std::errc::result_out_of_range) {
        return ERANGE;
    }
    return 0;
}
}
