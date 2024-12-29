#ifndef APPEND_H
#define APPEND_H

#include <stdint.h>
#include <stdlib.h>

#include "header.h"

#define MAX_NUM_CELLS 438


typedef enum {
    APPEND_OP_SUCCESS = 0,
    APPEND_OP_ERROR = -1,
    APPEND_OP_ERROR_INVALID_ARG = -2,
    APPEND_OP_ERROR_MEMORY_ALLOCATION = -3,
    APPEND_OP_ERROR_COL_DT_CELL_VALUE_MISMATCH = -4,
    APPEND_OP_ERROR_INVALID_FD = -5,
    APPEND_OP_INVALID_CELLS = -6,
    APPEND_OP_WRITE_ERROR = -7,
    APPEND_OP_READ_ERROR = -8
} AppendOpStatus;

typedef enum {
    CELL_TYPE_INT = 0,
    CELL_TYPE_FLOAT = 1,
    CELL_TYPE_STRING = 2
} cell_type_t;

typedef struct {
    size_t length;
    char *string;
} string_cell_t;

typedef union {
    int32_t int_value;
    float float_value;
    string_cell_t string_cell;
} cell_value_t;

typedef struct {
    cell_type_t type;
    cell_value_t data;
} cell_t;

typedef struct {
    size_t num_cells;
    cell_t *cells;
} row_t;

void free_row(row_t *row, size_t num_cells);
void print_parsed_row(row_t row, size_t num_cells);
AppendOpStatus parse_row(header_t header, char *row_in, row_t *row_out);
AppendOpStatus write_row(int fd, row_t row);
AppendOpStatus read_row(int fd, header_t header, row_t *row_out);

#endif