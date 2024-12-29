#ifndef HEADER_H
#define HEADER_H

#include <stdint.h>

#include "schema.h"

#define VERSION 1


typedef enum {
    HEADER_OP_SUCCESS = 0,
    HEADER_OP_ERROR_INVALID_FD = -1,
    HEADER_OP_INVALID_COLUMNS= -2,
    HEADER_OP_ERROR_INVALID_ARG = -3,
    HEADER_WRITE_ERROR = -4,
    HEADER_READ_ERROR = -5,
    HEADER_OP_ERROR_MEMORY_ALLOCATION = -6,
    HEADER_OP_READ_COLUMNS = -7,
    HEADER_OP_UPDATE_ERROR = -8
} HeaderOpStatus;

typedef struct {
    uint8_t magic[3];
    uint8_t version;
    size_t num_rows;
    size_t num_cols;
    column_t *columns;
} header_t;

void print_header(header_t header);
HeaderOpStatus initialize_header(column_t *columns, size_t num_cols, header_t *header_out);
HeaderOpStatus write_columns(int fd, column_t *columns, size_t num_cols);
HeaderOpStatus write_header(int fd, header_t header);
HeaderOpStatus read_header(int fd, header_t *header);
HeaderOpStatus update_header_num_rows(int fd, size_t increment, header_t *header);

#endif