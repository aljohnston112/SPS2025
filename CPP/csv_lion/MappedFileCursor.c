#include "MappedFileCursor.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void mapped_file_cursor_clean_up(const MappedFileCursor* self) {
    if (self->startPtr) {
        munmap(
            self->startPtr,
            self->endPtr - self->startPtr
        );
    }
    if (self->guardPtr) {
        munmap(
            self->guardPtr,
            get_page_size()
        );
    }
}

/**
 * Loads a file into memory.
 * @param self
 * @param filename
 * @return If open, fstat, mmap fail, then errno will be returned, else 0.
 */
int mapped_file_cursor_map_file(MappedFileCursor* self, const char* filename) {
    const int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return errno;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("Error getting file stats");
        close(fd);
        return errno;
    }

    if (st.st_size == 0) {
        close(fd);
        return -1;
    }

    const unsigned long page_size = get_page_size();
    const unsigned long page_mask = page_size - 1;
    const size_t rounded = (st.st_size + page_mask) & ~page_mask;

    char* startp = mmap(
        NULL,
        rounded + page_size,
        PROT_READ,
        MAP_ANON | MAP_PRIVATE,
        -1,
        0
    );
    if (startp == MAP_FAILED) {
        perror("Error mapping anonymous memory");
        close(fd);
        return errno;
    }

    self->guardPtr = startp + rounded;

    self->startPtr = mmap(
        startp,
        st.st_size,
        PROT_READ,
        MAP_SHARED | MAP_FIXED,
        fd,
        0
    );

    if (self->startPtr == MAP_FAILED) {
        perror("Error mapping file");
        munmap(startp, rounded + page_size);
        close(fd);
        return errno;
    }

    close(fd);

    self->endPtr = self->startPtr + st.st_size;
    self->currentPtr = self->startPtr;
    return 0;
}
