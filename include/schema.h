#ifndef SCHEMA_H
#define SCHEMA_H


#include <stdint.h>

#define MAX_COLUMN_NAME_LENGTH 255
#define MAX_DATA_TYPE_NAME_LENGTH 7
#define MAX_SCHEMA_LENGTH 2631


typedef enum {
    SCHEMA_OP_SUCCESS = 0,
    SCHEMA_OP_ERROR_INVALID_ARG = -1,
    SCHEMA_OP_ERROR_MEMORY_ALLOCATION = -2
} SchemaOpStatus;

typedef struct {
    char *name;
    uint16_t name_length;
    uint8_t data_type;
} column_t;

void free_columns(column_t *columns, size_t allocated_columns);
void print_parsed_schema(column_t *columns, size_t allocated_columns);
SchemaOpStatus parse_schema(char *schema, column_t **columns_out, size_t *allocated_columns_out);

#endif