#include "MappedFileCursor.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void mapped_file_cursor_clean_up(const MappedFileCursor* self) {
    if (self->startPtr) {
        munmap(
            self->startPtr,
            (size_t)(self->endPtr - self->startPtr)
        );
    }
    if (self->guardPtr) {
        munmap(
            self->guardPtr,
            (size_t)get_page_size()
        );
    }
}

/**
 * Loads a file into memory.
 * @param self
 * @param filename
 * @return If open, fstat, mmap fail, false, else true.
 */
int mapped_file_cursor_map_file(MappedFileCursor* self, const char* filename) {
    const int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return false;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("Error getting file stats");
        close(fd);
        return -1;
    }

    if (st.st_size == 0) {
        close(fd);
        return 1;
    }

    const int long page_size = get_page_size();
    const long int page_mask = page_size - 1;
    const long int rounded = (st.st_size + page_mask) & ~page_mask;

    char* startp = mmap(
        NULL,
        (size_t)(rounded + page_size),
        PROT_READ,
        MAP_ANON | MAP_PRIVATE,
        -1,
        0
    );
    if (startp == MAP_FAILED) {
        perror("Error mapping anonymous memory");
        close(fd);
        return -1;
    }

    self->guardPtr = startp + rounded;

    self->startPtr = mmap(
        startp,
        (size_t)st.st_size,
        PROT_READ,
        MAP_SHARED | MAP_FIXED,
        fd,
        0
    );

    if (self->startPtr == MAP_FAILED) {
        perror("Error mapping file");
        munmap(startp, (size_t)(rounded + page_size));
        close(fd);
        return -1;
    }

    close(fd);

    self->endPtr = self->startPtr + st.st_size;
    self->currentPtr = self->startPtr;
    return 0;
}
