#ifndef FILE_H
#define FILE_H


typedef enum {
    FILE_SUCCESS = 0,
    NULL_FILEPATH = -1,
    FILE_ERROR_EXISTS = -2,
    FILE_ERROR_CREATE = -3,
    FILE_ERROR_OPEN = -4
} FileOpStatus;


FileOpStatus create_file(const char *filepath, int *fd_out);
FileOpStatus open_file(const char *filepath, int *fd_out);

#endif