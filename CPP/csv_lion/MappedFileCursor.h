#ifndef MAPPEDFILECURSOR_H
#define MAPPEDFILECURSOR_H

#include <unistd.h>

typedef struct MappedFileCursor {
    char* startPtr;
    char* endPtr;
    char* currentPtr;
    char* guardPtr;
} MappedFileCursor;

void mapped_file_cursor_clean_up(const MappedFileCursor* self);

int mapped_file_cursor_map_file(MappedFileCursor* self, const char* filename);

static inline long get_page_size() {
    return sysconf(_SC_PAGESIZE);
}

static inline char* mapped_file_cursor_buf(const MappedFileCursor* self) {
    return self->currentPtr;
}

static inline long int mapped_file_cursor_size(const MappedFileCursor* self) {
    return self->endPtr - self->currentPtr;
}


static inline void mapped_file_cursor_consume(MappedFileCursor* self, const long int n) {
    self->currentPtr += (n < mapped_file_cursor_size(self)) ? n : mapped_file_cursor_size(self);
}

#endif //MAPPEDFILECURSOR_H
