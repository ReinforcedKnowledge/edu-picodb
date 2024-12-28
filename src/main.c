#include <stdio.h>
#include <unistd.h>

#include "file.h"
#include "schema.h"


int main(int argc, char *argv[]) {
    int newfile = 0;
    char *filepath = NULL;
    char *schema = NULL;
    char *row = NULL;
    
    int opt;
    char *optstring = ":f:ns:a:";
    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
            case 'f':
                filepath = optarg;
                break;
            case 'n':
                newfile = 1;
                break;
            case 's':
                schema = optarg;
                break;
            case 'a':
                row = optarg;
                break;
            case ':':
                fprintf(stderr, "Missing argument for option: -%c\n", optopt);
                return -1;
            case '?':
                fprintf(stderr, "Unknown option: -%c\n", optopt);
                return -1;
        }
    }

    if (!filepath) {
        fprintf(stderr, "File path is missing\n");
        return -1;
    }

    FileOpStatus fop_status;
    int fd = -1;
    if (newfile) {
        fop_status = create_file(filepath, &fd);
    } else {
        fop_status = open_file(filepath, &fd);
    }
    if (fop_status != FILE_SUCCESS) {
        switch (fop_status) {
            case NULL_FILEPATH:
                fprintf(stderr, "Empty filepath.\n");
                break;
            case FILE_ERROR_EXISTS:
                fprintf(stderr, "File already exists.\n");
                break;
            case FILE_ERROR_CREATE:
                fprintf(stderr, "File creation failed.\n");
                break;
            case FILE_ERROR_OPEN:
                fprintf(stderr, "File opening failed.\n");
                break;
            default:
                fprintf(stderr, "An unknown error occurred when creating/opening the file.\n");
                break;
        }
        return -1;
    }

    if (schema) {
        SchemaOpStatus sop_status;
        column_t *columns = NULL;
        size_t allocated_columns;
        sop_status = parse_schema(schema, &columns, &allocated_columns);

        if (sop_status != SCHEMA_OP_SUCCESS) {
            switch (sop_status) {
                case SCHEMA_OP_ERROR_INVALID_ARG:
                    fprintf(stderr, "The provided schema is malformatted.\n");
                    break;
                case SCHEMA_OP_ERROR_MEMORY_ALLOCATION:
                    fprintf(stderr, "Couldn't allocate memory when parsing the schema.\n");
                    break;
                default:
                    fprintf(stderr, "An unknown error occurred when parsing the schema.\n");
                    break;
            }
            return -1;
        }

        printf("Parsed schema:\n\n");
        print_parsed_schema(columns, allocated_columns);
    }

    if (row) {
        // Placeholder
    }

    if (close(fd) == -1) {
        fprintf(stderr, "File closing failed.\n");
        return -1;
    }

    return 0;
}
