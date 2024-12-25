#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int newfile = 0;
    char *filepath = NULL;
    char *schema = NULL;
    char *row_in = NULL;
    
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
                row_in = optarg;
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

    if (newfile) {
        // Placeholder
    } else {
        // Placeholder
    }

    if (schema) {
        // Placeholder
    }

    if (row_in) {
        // Placeholder
    }

    return 0;
}
