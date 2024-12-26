#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "file.h"


FileOpStatus create_file(const char *filepath, int *fd_out) {
    if (filepath == NULL) {
        return NULL_FILEPATH;
    }

    int fd = open(filepath, O_RDWR | O_CREAT | O_EXCL, 0644);

    if (fd == -1) {
        if (errno == EEXIST) {
            return FILE_ERROR_EXISTS;
        } else {
            return FILE_ERROR_CREATE;
        }
    }

    *fd_out = fd;
    return FILE_SUCCESS;
}

FileOpStatus open_file(const char *filepath, int *fd_out) {
    if (filepath == NULL) {
        return NULL_FILEPATH;
    }

    int fd = open(filepath, O_RDWR);
    if (fd == -1) {
        return FILE_ERROR_OPEN;
    }

    *fd_out = fd;
    return FILE_SUCCESS;
}