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

static long get_page_size() {
    return sysconf(_SC_PAGESIZE);
}

static char* mapped_file_cursor_buf(const MappedFileCursor* self) {
    return self->currentPtr;
}

static size_t mapped_file_cursor_size(const MappedFileCursor* self) {
    return self->endPtr - self->currentPtr;
}


static void mapped_file_cursor_consume(MappedFileCursor* self, const size_t n) {
    self->currentPtr += (n < mapped_file_cursor_size(self)) ? n : mapped_file_cursor_size(self);
}

#endif //MAPPEDFILECURSOR_H
