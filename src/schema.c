#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h> 

#include "schema.h"


void free_columns(column_t *columns, size_t allocated_columns) {
    for (size_t i = 0; i < allocated_columns; i++) {
        free(columns[i].name);
    }
    free(columns);
}

void print_parsed_schema(column_t *columns, size_t allocated_columns) {
    for (size_t i = 0; i < allocated_columns; i++) {
        printf("Column name: %s\n", columns[i].name);
        uint8_t dt = columns[i].data_type;
        if (dt == 0) {
            printf("  Data type: int\n");
        } else if (dt == 1) {
            printf("  Data type: float\n");
        } else if (dt == 2) {
            printf("  Data type: string\n");
        } else {
            printf("  Unrecognized data type\n");
        }
        printf("\n");
    }
}

SchemaOpStatus parse_schema(char *schema, column_t **columns_out, size_t *allocated_columns_out) {
    if (schema[0] != '(') {
        return SCHEMA_OP_ERROR_INVALID_ARG;
    }

    column_t *columns = NULL; 
    size_t num_columns = 10; 
    columns = (column_t *) calloc(num_columns, sizeof(column_t)); 
    if (columns == NULL) {
        return SCHEMA_OP_ERROR_MEMORY_ALLOCATION;
    }

    size_t col_num = 0;  // Determines when to realloc
    size_t realloc_counter = 1;

    size_t i = 1;
    size_t j = 1;
    uint8_t ch = (uint8_t) schema[1];
    char column_name_start = 1;
    char data_type_start = 0;

    while (ch != 41 && ch != '\0' && i < MAX_SCHEMA_LENGTH) {
        if (column_name_start) {
            if (ch < 65 || (ch > 90 && ch < 97 && ch != 95) || ch > 122 ) {
                // First character of a column name should only be letters or underscore
                free_columns(columns, col_num);
                return SCHEMA_OP_ERROR_INVALID_ARG;
            }

            column_name_start = 0;
            i++;
            ch = (uint8_t) schema[i];
        } else {
            if (!data_type_start) {
                if (ch < 48 || (ch > 57 && ch < 65 && ch != 58) || (ch > 90 && ch < 97 && ch != 95) || ch > 122) {
                    free_columns(columns, col_num);
                    return SCHEMA_OP_ERROR_INVALID_ARG; 
                }

                if (ch == 58) {
                    size_t column_name_length = i - j;
                    if ((column_name_length + 1) > MAX_COLUMN_NAME_LENGTH) {
                        free_columns(columns, col_num);
                        return SCHEMA_OP_ERROR_INVALID_ARG; 
                    }

                    columns[col_num].name_length = column_name_length;
                    columns[col_num].name = (char *) malloc(columns[col_num].name_length + 1);  // Pay attention to the null termination symbol here
                    if (columns[col_num].name == NULL) {
                        free_columns(columns, col_num);
                        return SCHEMA_OP_ERROR_MEMORY_ALLOCATION;
                    }
                    memcpy(columns[col_num].name, &schema[j], column_name_length);
                    columns[col_num].name[column_name_length] = '\0';   // Pay attention and null-terminate the string
                    
                    j = i + 1;  // j is start of data type
                    data_type_start = 1;
                }

                i++;
                ch = (uint8_t) schema[i];
            } else {
                if (ch == 32) {
                    size_t data_type_length = i - j;
                    if ((data_type_length + 1) > MAX_DATA_TYPE_NAME_LENGTH) {
                        free_columns(columns, col_num);
                        return SCHEMA_OP_ERROR_INVALID_ARG; 
                    }

                    if (data_type_length == 3 && memcmp(&schema[j], "int", 3) == 0) {
                        columns[col_num].data_type = 0;
                    } else if (data_type_length == 5 && memcmp(&schema[j], "float", 5) == 0) {
                        columns[col_num].data_type = 1;
                    } else if (data_type_length == 6 && memcmp(&schema[j], "string", 6) == 0) {
                        columns[col_num].data_type = 2;
                    } else {
                        free_columns(columns, col_num);
                        return SCHEMA_OP_ERROR_INVALID_ARG; 
                    }

                    col_num++;
                    if ((col_num + 1) == (num_columns * realloc_counter)) {
                        column_t *temp_columns = reallocarray(columns, num_columns * (realloc_counter + 1), sizeof(column_t));
                        if (temp_columns == NULL) {
                            free_columns(columns, col_num - 1);
                            return SCHEMA_OP_ERROR_MEMORY_ALLOCATION;
                        } else {
                            columns = temp_columns;
                        }
                        realloc_counter++;
                    }

                    data_type_start = 0;
                    column_name_start = 1;
                    j = i + 1;  // j is the start of column name
                } else {
                    if (ch < 97 || ch > 122) {
                        free_columns(columns, col_num);
                        return SCHEMA_OP_ERROR_INVALID_ARG; 
                    }
                }

                i++;
                ch = (uint8_t) schema[i];
            }
        }
    }

    if (MAX_SCHEMA_LENGTH <= i) {
        free_columns(columns, col_num);
        return SCHEMA_OP_ERROR_INVALID_ARG;
    }

    // Case not handled within loop because of termination
    size_t data_type_length = i - j;
    if ((data_type_length + 1) > MAX_DATA_TYPE_NAME_LENGTH) {
        free_columns(columns, col_num);
        return SCHEMA_OP_ERROR_INVALID_ARG; 
    }

    if (data_type_length == 3 && memcmp(&schema[j], "int", 3) == 0) {
        columns[col_num].data_type = 0;
    } else if (data_type_length == 5 && memcmp(&schema[j], "float", 5) == 0) {
        columns[col_num].data_type = 1;
    } else if (data_type_length == 6 && memcmp(&schema[j], "string", 6) == 0) {
        columns[col_num].data_type = 2;
    } else {
        free_columns(columns, col_num);
        return SCHEMA_OP_ERROR_INVALID_ARG; 
    }

    *columns_out = columns;
    *allocated_columns_out = col_num + 1;  // "Actual" number of allocated columns
    return SCHEMA_OP_SUCCESS;
}

