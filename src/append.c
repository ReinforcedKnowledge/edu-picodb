#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "append.h"
#include "header.h"


uint32_t float_to_network_bytes(float value) {
    uint32_t temp;
    memcpy(&temp, &value, sizeof(temp));
    temp = htonl(temp);
    return temp;
}

float network_bytes_to_float(uint32_t net_value) {
    float result;
    net_value = ntohl(net_value);
    memcpy(&result, &net_value, sizeof(result));
    return result;
}

void free_row(row_t *row, size_t num_cells) {
    for (size_t i = 0; i < num_cells; i++) {
        uint8_t dt = row->cells[i].type;
        if (dt == CELL_TYPE_STRING) {
            free(row->cells[i].data.string_cell.string);
        }
    }
    free(row->cells);
}

void print_parsed_row(row_t row, size_t num_cells) {
    for (size_t i = 0; i < num_cells; i++) {
        printf("Cell number: %ld\n", i + 1);
        uint8_t dt = row.cells[i].type;
        if (dt == CELL_TYPE_INT) {
            printf("\tData type: int\n");
            printf("\tValue: %d\n", row.cells[i].data.int_value);
        } else if (dt == CELL_TYPE_FLOAT) {
            printf("\tData type: float\n");
            printf("\tValue: %f\n", row.cells[i].data.float_value);
        } else if (dt == CELL_TYPE_STRING) {
            printf("\tData type: string\n");
            printf("\tValue: %s\n", row.cells[i].data.string_cell.string);
            printf("\tLength: %ld\n", row.cells[i].data.string_cell.length);
        } else {
            printf("  Unrecognized data type\n");
        }
        printf("\n");
    }
}

AppendOpStatus parse_row(header_t header, char *row_in, row_t *row_out) {
    if (row_in[0] != '(') {
        return APPEND_OP_ERROR_INVALID_ARG;
    }

    row_t row;
    size_t initial_num_cells = 10;
    row.cells = (cell_t *) calloc(initial_num_cells, sizeof(cell_t));
    if (row.cells == NULL) {
        return APPEND_OP_ERROR_MEMORY_ALLOCATION;
    }

    size_t cell_num = 0;  // Determines when to realloc and the hard limit on cell number
    size_t realloc_counter = 1;

    size_t i = 1;
    size_t j = 1;
    uint8_t ch = (uint8_t) row_in[i];

    uint8_t is_int = 1;
    uint8_t is_float = 0;
    uint8_t is_string = 0;

    while (ch != 41 && ch != '\0' && cell_num < MAX_NUM_CELLS) {
        if (cell_num >= header.num_cols) {
            // Important to avoid out of bound reads
            free_row(&row, cell_num);
            return APPEND_OP_ERROR_INVALID_ARG;
        }

        if (ch == 32 && (uint8_t) row_in[i + 1] == 38 && (uint8_t) row_in[i + 2] == 38 && (uint8_t) row_in[i + 3] == 32) {
            // Looking for: ' && '    
            size_t cell_value_length = i - j;
            char *cell_value = (char *) malloc(cell_value_length + 1); 
            if (cell_value == NULL) {
                free_row(&row, cell_num);
                return APPEND_OP_ERROR_MEMORY_ALLOCATION;
            }
            memcpy(cell_value, &row_in[j], cell_value_length);
            cell_value[cell_value_length] = '\0';

            if (is_string) {
                if (header.columns[cell_num].data_type == 2) {
                    row.cells[cell_num].type = CELL_TYPE_STRING;
                    row.cells[cell_num].data.string_cell.length = cell_value_length;
                    row.cells[cell_num].data.string_cell.string = (char *) malloc(cell_value_length + 1);
                    if (row.cells[cell_num].data.string_cell.string == NULL) {
                        free_row(&row, cell_num);
                        return APPEND_OP_ERROR_MEMORY_ALLOCATION;
                    }
                    memcpy(row.cells[cell_num].data.string_cell.string, cell_value, cell_value_length);
                    row.cells[cell_num].data.string_cell.string[cell_value_length] = '\0';
                } else {
                    free_row(&row, cell_num);
                    free(cell_value);
                    return APPEND_OP_ERROR_COL_DT_CELL_VALUE_MISMATCH;
                }
            } else if (is_float) {
                if (header.columns[cell_num].data_type == 1) {
                    row.cells[cell_num].type = CELL_TYPE_FLOAT;
                    row.cells[cell_num].data.float_value = atof(cell_value);
                } else {
                    free_row(&row, cell_num);
                    free(cell_value);
                    return APPEND_OP_ERROR_COL_DT_CELL_VALUE_MISMATCH;
                }
            } else if (is_int) {
                if (header.columns[cell_num].data_type == 0) {
                    row.cells[cell_num].type = CELL_TYPE_INT;
                    row.cells[cell_num].data.int_value = atol(cell_value);
                } else {
                    free_row(&row, cell_num);
                    free(cell_value);
                    return APPEND_OP_ERROR_COL_DT_CELL_VALUE_MISMATCH;
                }
            }

            free(cell_value);  // Don't forget this


            cell_num++;
            if ((cell_num + 1) == (initial_num_cells * realloc_counter)) {
                cell_t *temp_cells = reallocarray(row.cells, initial_num_cells * (realloc_counter + 1), sizeof(cell_t));
                if (temp_cells == NULL) {
                    free_row(&row, cell_num);
                    return APPEND_OP_ERROR_MEMORY_ALLOCATION;
                } else {
                    row.cells = temp_cells;
                }
                realloc_counter++;
            }

            is_int = 1;
            is_float = 0;
            is_string = 0;

            i = i + 4;
            j = i;
            ch = (uint8_t) row_in[i];
            continue;
        }

        if (ch < 48 || ch > 57) {
            if (ch == 46) {
                if (is_float) {
                    is_string = 1;
                } else {
                    is_float = 1;
                }
            } else {
                is_string = 1;
            }
        }
        
        i++;
        ch = (uint8_t) row_in[i];
    }


    if (MAX_NUM_CELLS <= cell_num) {
        free_row(&row, cell_num);
        return APPEND_OP_ERROR_INVALID_ARG;
    }


    size_t cell_value_length = i - j;
    char *cell_value = (char *) malloc(cell_value_length + 1);
    if (cell_value == NULL) {
        free_row(&row, cell_num);
        return APPEND_OP_ERROR_MEMORY_ALLOCATION;
    }
    memcpy(cell_value, &row_in[j], cell_value_length);
    cell_value[cell_value_length] = '\0';

    if (is_string) {
        if (header.columns[cell_num].data_type == 2) {
            row.cells[cell_num].type = CELL_TYPE_STRING;
            row.cells[cell_num].data.string_cell.length = cell_value_length;
            row.cells[cell_num].data.string_cell.string = (char *) malloc(cell_value_length + 1);
            if (row.cells[cell_num].data.string_cell.string == NULL) {
                free_row(&row, cell_num);
                return APPEND_OP_ERROR_MEMORY_ALLOCATION;
            }
            memcpy(row.cells[cell_num].data.string_cell.string, cell_value, cell_value_length);
            row.cells[cell_num].data.string_cell.string[cell_value_length] = '\0';
        } else {
            free_row(&row, cell_num);
            free(cell_value);
            return APPEND_OP_ERROR_COL_DT_CELL_VALUE_MISMATCH;
        }
    } else if (is_float) {
        if (header.columns[cell_num].data_type == 1) {
            row.cells[cell_num].type = CELL_TYPE_FLOAT;
            row.cells[cell_num].data.float_value = atof(cell_value);
        } else {
            free_row(&row, cell_num);
            free(cell_value);
            return APPEND_OP_ERROR_COL_DT_CELL_VALUE_MISMATCH;
        }
    } else if (is_int) {
        if (header.columns[cell_num].data_type == 0) {
            row.cells[cell_num].type = CELL_TYPE_INT;
            row.cells[cell_num].data.int_value = atol(cell_value);
        } else {
            free_row(&row, cell_num);
            free(cell_value);
            return APPEND_OP_ERROR_COL_DT_CELL_VALUE_MISMATCH;
        }
    }

    free(cell_value);

    if (cell_num + 1 != header.num_cols) {
        return APPEND_OP_ERROR_INVALID_ARG;
    }

    row.num_cells = cell_num + 1;
    *row_out = row; 
    return APPEND_OP_SUCCESS;
}

AppendOpStatus write_row(int fd, row_t row) {
    if (fd < 0) {
        return APPEND_OP_ERROR_INVALID_FD;
    }

    if (row.cells == NULL) {
        return APPEND_OP_INVALID_CELLS;
    }

    ssize_t bytes_writen;


    for (size_t i = 0; i < row.num_cells; i++) {
        uint8_t dt = (uint8_t) row.cells[i].type;
        bytes_writen = write(fd, &dt, sizeof(uint8_t));
        if (bytes_writen != sizeof(uint8_t)) {
            return APPEND_OP_WRITE_ERROR;
        }

        if (dt == CELL_TYPE_INT) {
            uint32_t int_value_nbo = htonl(row.cells[i].data.int_value);
            bytes_writen = write(fd, &int_value_nbo, sizeof(uint32_t));
            if (bytes_writen != sizeof(uint32_t)) {
                return APPEND_OP_WRITE_ERROR;
            }
        } else if (dt == CELL_TYPE_FLOAT) {
            uint32_t float_value_nbo = float_to_network_bytes(row.cells[i].data.float_value);
            bytes_writen = write(fd, &float_value_nbo, sizeof(uint32_t));
            if (bytes_writen != sizeof(uint32_t)) {
                return APPEND_OP_WRITE_ERROR;
            }
        } else if (dt == CELL_TYPE_STRING) {
            uint32_t length = htonl(row.cells[i].data.string_cell.length);
            bytes_writen = write(fd, &length, sizeof(uint32_t));
            if (bytes_writen != sizeof(uint32_t)) {
                return APPEND_OP_WRITE_ERROR;
            }
            bytes_writen = write(fd, row.cells[i].data.string_cell.string, row.cells[i].data.string_cell.length);
            if (bytes_writen < 0) {
                return APPEND_OP_WRITE_ERROR;
            }
            if ((size_t)bytes_writen != row.cells[i].data.string_cell.length) {
                return APPEND_OP_WRITE_ERROR;
            }
        }
    }

    return APPEND_OP_SUCCESS;
}

AppendOpStatus read_row(int fd, header_t header, row_t *row_out) {
    if (fd < 0) {
        return APPEND_OP_ERROR_INVALID_FD;
    }

    uint32_t num_cols = header.num_cols;

    row_t row;
    row.num_cells = num_cols;
    row.cells = (cell_t *) calloc(num_cols, sizeof(cell_t));

    ssize_t bytes_read;

    for (uint32_t cell_it = 0; cell_it < num_cols; cell_it++) {
        bytes_read = read(fd, &row.cells[cell_it].type, sizeof(uint8_t));
        if (row.cells[cell_it].type == CELL_TYPE_INT) {
            bytes_read = read(fd, &row.cells[cell_it].data.int_value, sizeof(uint32_t));
            if (bytes_read != sizeof(uint32_t)) {
                free_row(&row, cell_it);
                return APPEND_OP_READ_ERROR;
            }
            row.cells[cell_it].data.int_value = ntohl(row.cells[cell_it].data.int_value);
        } else if (row.cells[cell_it].type == CELL_TYPE_FLOAT) {
            uint32_t float_value_nbo;
            bytes_read = read(fd, &float_value_nbo, sizeof(uint32_t));
            if (bytes_read != sizeof(uint32_t)) {
                free_row(&row, cell_it);
                return APPEND_OP_READ_ERROR;
            }
            row.cells[cell_it].data.float_value = network_bytes_to_float(float_value_nbo);
        } else if (row.cells[cell_it].type == CELL_TYPE_STRING) {
            bytes_read = read(fd, &row.cells[cell_it].data.string_cell.length, sizeof(uint32_t));
            if (bytes_read != sizeof(uint32_t)) {
                free_row(&row, cell_it);
                return APPEND_OP_READ_ERROR;
            }
            row.cells[cell_it].data.string_cell.length = ntohl(row.cells[cell_it].data.string_cell.length);
            row.cells[cell_it].data.string_cell.string = (char *) malloc(row.cells[cell_it].data.string_cell.length + 1);
            if (row.cells[cell_it].data.string_cell.string == NULL) {
                free_row(&row, cell_it);
                return APPEND_OP_ERROR_MEMORY_ALLOCATION;
            }
            bytes_read = read(fd, row.cells[cell_it].data.string_cell.string, sizeof(char) * row.cells[cell_it].data.string_cell.length);
            if (bytes_read < 0) {
                free_row(&row, cell_it);
                return APPEND_OP_WRITE_ERROR;
            }
            if ((size_t)bytes_read != sizeof(char) * row.cells[cell_it].data.string_cell.length) {
                free_row(&row, cell_it);
                return APPEND_OP_READ_ERROR;
            }
            row.cells[cell_it].data.string_cell.string[row.cells[cell_it].data.string_cell.length] = '\0';
        }
    }

    *row_out = row;

    return APPEND_OP_SUCCESS;
}