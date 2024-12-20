#ifndef MEMORY_UTIL_H
#define MEMORY_UTIL_H

#include <stddef.h>

void* safe_malloc(size_t size);
void safe_free(void** ptr);



#endif //MEMORY_UTIL_H
