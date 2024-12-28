#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "header.h"


void print_header(header_t header) {
    printf("Magic: %s\n", header.magic);
    printf("Version: %u\n", header.version);
    printf("Number of rows: %zu\n", header.num_rows);
    printf("Number of columns: %zu\n", header.num_cols);

    print_parsed_schema(header.columns, header.num_cols);
}

int validate_magic(uint8_t magic[3], ssize_t bytes_read) {
    if (bytes_read != sizeof(uint8_t) * 3) {
        return 0;
    }

    if (memcmp(magic, "rfk", sizeof(uint8_t) * 3) != 0) {
        return 0;
    }

    return 1;
}

int validate_version(uint8_t version, ssize_t bytes_read) {
    if (bytes_read != sizeof(uint8_t)) {
        return 0;
    }

    if (version != VERSION) {
        return 0;
    }

    return 1;
}

HeaderOpStatus initialize_header(column_t *columns, size_t num_cols, header_t *header_out) {
    if (columns == NULL || num_cols == 0) {
        return HEADER_OP_INVALID_COLUMNS;
    }

    header_t header = {
        .magic = {0x72, 0x66, 0x6b},  // 'r', 'f', 'k'
        .version = 1,
        .num_rows = 0,
        .num_cols = num_cols,
        .columns = columns
    };
    *header_out = header;
    return HEADER_OP_SUCCESS;
}

HeaderOpStatus write_columns(int fd, column_t *columns, size_t num_cols) {
    if (fd < 0 || columns == NULL || num_cols == 0) {
        if (fd < 0) {
            return HEADER_OP_ERROR_INVALID_FD;
        }

        if (columns == NULL || num_cols == 0) {
            return HEADER_OP_INVALID_COLUMNS;
        }
    }

    ssize_t bytes_written;

    for (size_t i = 0; i < num_cols; i++) {
        uint16_t name_length_nbo = htons(columns[i].name_length);
        bytes_written = write(fd, &name_length_nbo, sizeof(uint16_t));
        if (bytes_written != sizeof(uint16_t)) {
            return HEADER_WRITE_ERROR;
        }
        bytes_written = write(fd, columns[i].name, sizeof(char) * columns[i].name_length);
        if (bytes_written < 0) {
            return HEADER_WRITE_ERROR;
        }
        if ((size_t)bytes_written != sizeof(char) * columns[i].name_length) {
            return HEADER_WRITE_ERROR;
        }
        bytes_written = write(fd, &columns[i].data_type, sizeof(uint8_t));
        if (bytes_written != sizeof(uint8_t)) {
            return HEADER_WRITE_ERROR;
        }
    }

    return HEADER_OP_SUCCESS;
}

HeaderOpStatus read_columns(int fd, size_t num_cols, column_t **columns_out) {
    if (num_cols == 0) {
        return HEADER_OP_INVALID_COLUMNS;
    }

    ssize_t bytes_read;
    
    column_t *columns = NULL; 
    columns = (column_t *) calloc(num_cols, sizeof(column_t));
    if (columns == NULL) {
        return HEADER_OP_ERROR_MEMORY_ALLOCATION;
    }

    for (size_t i = 0; i < num_cols; i++) {
        bytes_read = read(fd, &columns[i].name_length, sizeof(uint16_t));
        if (bytes_read != sizeof(uint16_t)) {
            free_columns(columns, i);
            return HEADER_OP_READ_COLUMNS;
        }
        columns[i].name_length = ntohs(columns[i].name_length);

        columns[i].name = (char *) malloc(columns[i].name_length + 1);  // Pay attention to the null termination symbol here
        if (columns[i].name == NULL) {
            free_columns(columns, i);
            return HEADER_OP_ERROR_MEMORY_ALLOCATION;
        }
        bytes_read = read(fd, columns[i].name, sizeof(char) * columns[i].name_length);
        if (bytes_read < 0) {
            return HEADER_READ_ERROR;
        }
        if ((size_t)bytes_read != sizeof(char) * columns[i].name_length) {
            free_columns(columns, i);
            return HEADER_OP_READ_COLUMNS;
        }

        bytes_read = read(fd, &columns[i].data_type, sizeof(uint8_t));
        if (bytes_read != sizeof(uint8_t)) {
            free_columns(columns, i);
            return HEADER_OP_READ_COLUMNS;
        }
    }

    *columns_out = columns;
    return HEADER_OP_SUCCESS;
}

HeaderOpStatus write_header(int fd, header_t header) {
    if (fd < 0) {
        return HEADER_OP_ERROR_INVALID_FD;
    }

    if (header.columns == NULL) {
        return HEADER_OP_INVALID_COLUMNS;
    }

    ssize_t bytes_written;

    bytes_written = write(fd, header.magic, sizeof(header.magic));
    if (bytes_written != sizeof(header.magic)) {
        return HEADER_WRITE_ERROR;
    }

    bytes_written = write(fd, &header.version, sizeof(header.version));
    if (bytes_written != sizeof(header.version)) {
        return HEADER_WRITE_ERROR;
    }

    size_t num_rows_nbo = htonl(header.num_rows);
    bytes_written = write(fd, &num_rows_nbo, sizeof(header.num_rows));
    if (bytes_written != sizeof(header.num_rows)) {
        return HEADER_WRITE_ERROR;
    }

    size_t num_cols_nbo = htonl(header.num_cols);
    bytes_written = write(fd, &num_cols_nbo, sizeof(header.num_cols));
    if (bytes_written != sizeof(header.num_rows)) {
        return HEADER_WRITE_ERROR;
    }

    HeaderOpStatus write_cols_status;
    write_cols_status = write_columns(fd, header.columns, header.num_cols);

    return write_cols_status;
}

HeaderOpStatus read_header(int fd, header_t *header_out) {
    if (fd < 0) {
        return HEADER_OP_ERROR_INVALID_FD;
    }

    ssize_t bytes_read;
    header_t header;
    
    bytes_read = read(fd, header.magic, sizeof(uint8_t) * 3);
    if (!validate_magic(header.magic, bytes_read)) {
        return HEADER_READ_ERROR;
    }

    bytes_read = read(fd, &header.version, sizeof(uint8_t));
    if (!validate_version(header.version, bytes_read)) {
        return HEADER_READ_ERROR;
    }

    bytes_read = read(fd, &header.num_rows, sizeof(size_t));
    if (bytes_read != sizeof(size_t)) {
        return HEADER_READ_ERROR;
    }
    header.num_rows = ntohl(header.num_rows);

    bytes_read = read(fd, &header.num_cols, sizeof(size_t));
    if (bytes_read != sizeof(size_t)) {
        return HEADER_READ_ERROR;
    }
    header.num_cols = ntohl(header.num_cols);

    HeaderOpStatus read_cols_status;
    read_cols_status = read_columns(fd, header.num_cols, &header.columns);
    if (read_cols_status != HEADER_OP_SUCCESS) {
        return read_cols_status;
    }

    *header_out = header;

    return HEADER_OP_SUCCESS;
}

HeaderOpStatus update_header_num_rows(int fd, size_t increment, header_t *header) {
    if (fd < 0 || header == NULL) {
        return HEADER_OP_ERROR_INVALID_ARG;
    }

    ssize_t bytes_written;

    off_t current_offset = lseek(fd, 0, SEEK_CUR);
    if (current_offset == (off_t)-1) {
        return HEADER_OP_UPDATE_ERROR;
    }

    off_t new_offset = lseek(fd, sizeof(header->magic) + sizeof(header->version), SEEK_SET);
    if (new_offset == (off_t)-1) {
        return HEADER_OP_UPDATE_ERROR;
    }

    size_t num_rows_nbo = htonl(header->num_rows);
    bytes_written = write(fd, &num_rows_nbo, sizeof(size_t));
    if (bytes_written != sizeof(size_t)) {
        return HEADER_OP_UPDATE_ERROR;
    }

    if (lseek(fd, current_offset, SEEK_SET) == (off_t)-1) {
        return HEADER_OP_UPDATE_ERROR;
    }

    header->num_rows = header->num_rows + increment;
    return HEADER_OP_SUCCESS;
}