#include <stdlib.h>
#include "CsvCell.h"

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <string.h>

/**
 * The caller is responsible for deallocating the returned string using `free()`.
 * errno may be set to ENOMEM if there is no memory left.
 * @param self
 * @return A null-terminated string that must be deallocated when no longer needed.
 */
char* csv_cell_as_str(const CsvCell* self) {
    // +1 for null
    char* str = malloc(self->size + 1);
    if (str == NULL) {
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
    if (errno != 0)
    {
        return -errno;
    }
    if (value > INT_MAX || value < INT_MIN) {
        return ERANGE;
    }
    if ((size_t)(endptr - self->ptr) != self->size) {
        return EINVAL;
    }
    *out_value = (int)value;
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
    char* endptr;
    const double value = strtod(self->ptr, &endptr);
    if (errno != 0)
    {
        return -errno;
    }
    if (value > DBL_MAX || value < DBL_MAX) {
        return ERANGE;
    }
    if ((size_t)(endptr - self->ptr) != self->size) {
        return EINVAL;
    }
    *out_value = (double)value;
    return errno;
}