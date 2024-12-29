#include <stdio.h>
#include <unistd.h>

#include "file.h"
#include "schema.h"
#include "header.h"
#include "append.h"


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

    if (schema && !newfile) {
        printf("You can't specify a new schema for an already existing file.\n");
        if (close(fd) == -1) {
            fprintf(stderr, "Failed to close the file..\n");
        }
        return -1;
    }

    if (schema && newfile) {
        // Schema parsing
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

        // Header initialization
        HeaderOpStatus hop_status;
        header_t header;
        hop_status = initialize_header(columns, allocated_columns, &header);
        if (hop_status != HEADER_OP_SUCCESS) {
            fprintf(stderr, "Failed to initialize header: invalid columns argument.\n");
            free_columns(columns, allocated_columns);
            if (close(fd) == -1) {
                fprintf(stderr, "Failed to close the file..\n");
            }
            return -1;
        }

        // Header writing
        hop_status = write_header(fd, header);
        if (hop_status != HEADER_OP_SUCCESS) {
            fprintf(stderr, "Failed to write header.\n");
            free_columns(columns, allocated_columns);
            if (close(fd) == -1) {
                fprintf(stderr, "Failed to close the file..\n");
            }
            return -1;
        }

        printf("Header to write:\n\n");
        print_header(header);


        // Test that our header was written correctly
#ifdef VERIFY_HEADER
        if (lseek(fd, 0, SEEK_SET) == -1) {
            fprintf(stderr, "Failed to seek in file.\n");
            free_columns(columns, allocated_columns);
            if (close(fd) == -1) {
                fprintf(stderr, "File closing failed.\n");
            }
            return -1;
        }

        header_t rheader;
        hop_status = read_header(fd, &rheader);
        if (hop_status != HEADER_OP_SUCCESS) {
            fprintf(stderr, "Failed to read header.\n");
            free_columns(columns, allocated_columns);
            if (close(fd) == -1) {
                fprintf(stderr, "File closing failed.\n");
            }
            return -1;
        }

        printf("Read header:\n\n");
        print_header(rheader);

        free_columns(rheader.columns, rheader.num_cols);
#endif // VERIFY_HEADER

        if (row) {
            // Append row
        }

        free_columns(columns, allocated_columns);
    }

    if (!schema && !newfile) {
        if (row) {
            // Read header
            HeaderOpStatus hop_status;
            header_t header;
            hop_status = read_header(fd, &header);
            if (hop_status != HEADER_OP_SUCCESS) {
                fprintf(stderr, "Failed to read header.\n");
                if (close(fd) == -1) {
                    fprintf(stderr, "File closing failed.\n");
                }
                return -1;
            }

            printf("Read header:\n\n");
            print_header(header);

            // Parse the row
            AppendOpStatus aop_status;
            row_t parsed_row;
            aop_status = parse_row(header, row, &parsed_row);
            if (aop_status != APPEND_OP_SUCCESS) {
                fprintf(stderr, "Failed to parse row.\n");
                free_columns(header.columns, header.num_cols);
                if (close(fd) == -1) {
                    fprintf(stderr, "File closing failed.\n");
                }
                return -1;
            }

            printf("Parsed row:\n\n");
            print_parsed_row(parsed_row, parsed_row.num_cells);

            // For safety, let's get to the end of the file, should have used O_APPEND...
            if (lseek(fd, 0, SEEK_END) == -1) {
                fprintf(stderr, "Failed to seek to end of file.\n");
                free_row(&parsed_row, parsed_row.num_cells);
                free_columns(header.columns, header.num_cols);
                if (close(fd) == -1) {
                    fprintf(stderr, "File closing failed.\n");
                }
                return -1;
            }

            // Write row
            aop_status = write_row(fd, parsed_row);
            if (aop_status != APPEND_OP_SUCCESS) {
                fprintf(stderr, "Failed to write row.\n");
                free_row(&parsed_row, parsed_row.num_cells);
                free_columns(header.columns, header.num_cols);
                if (close(fd) == -1) {
                    fprintf(stderr, "File closing failed.\n");
                }
                return -1;
            }

            // Update the header: both in-memory and also on disk.
            HeaderOpStatus huop_status;
            huop_status = update_header_num_rows(fd, 1, &header);
            if (huop_status != HEADER_OP_SUCCESS) {
                fprintf(stderr, "Failed to update the header.\n");
                free_row(&parsed_row, parsed_row.num_cells);
                free_columns(header.columns, header.num_cols);
                if (close(fd) == -1) {
                    fprintf(stderr, "File closing failed.\n");
                }
                return -1;
            }

            free_row(&parsed_row, parsed_row.num_cells);
            free_columns(header.columns, header.num_cols);

#ifdef VERIFY_ROW
            // Seek to start of file to read the header and the first row
            if (lseek(fd, 0, SEEK_SET) == -1) {
                fprintf(stderr, "Failed to seek in file.\n");
                if (close(fd) == -1) {
                    fprintf(stderr, "File closing failed.\n");
                }
                return -1;
            }

            // Read the header
            header_t verify_row_header;
            HeaderOpStatus verify_row_hop_status = read_header(fd, &verify_row_header);
            if (verify_row_hop_status != HEADER_OP_SUCCESS) {
                fprintf(stderr, "Failed to read header when verifying row.\n");
                free_row(&parsed_row, parsed_row.num_cells);
                free_columns(header.columns, header.num_cols);
                if (close(fd) == -1) {
                    fprintf(stderr, "File closing failed.\n");
                }
                return -1;
            }

            // Read the first row
            row_t first_row;
            AppendOpStatus aop_read_first_row_status = read_row(fd, verify_row_header, &first_row);
            if (aop_read_first_row_status != APPEND_OP_SUCCESS) {
                fprintf(stderr, "Failed to read row for verification.\n");
                free_row(&parsed_row, parsed_row.num_cells);
                free_columns(verify_row_header.columns, verify_row_header.num_cols);
                if (close(fd) == -1) {
                    fprintf(stderr, "File closing failed.\n");
                }
                return -1;
            }

            printf("Verified first row from file:\n\n");
            print_parsed_row(first_row, first_row.num_cells);

            free_row(&first_row, first_row.num_cells);
            free_columns(verify_row_header.columns, verify_row_header.num_cols);
#endif // VERIFY_ROW
        }
    }

    if (close(fd) == -1) {
        fprintf(stderr, "Failed to close the file.\n");
        return -1;
    }

    return 0;
}