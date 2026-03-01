#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(argc, argv)
    int argc;
    char *argv[];
{
    FILE *out_fp, *in_fp;
    char line[256];
    int i, len, is_last, is_null, is_eof;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <output.hex> <input1.hex> [input2.hex ...]\n", argv[0]);
        exit(1);
    }

    out_fp = fopen(argv[1], "w");
    if (out_fp == NULL) {
        fprintf(stderr, "Error: Could not open output file '%s'.\n", argv[1]);
        exit(1);
    }

    printf("Starting merge process...\n");
    printf("Output destination: %s\n", argv[1]);

    for (i = 2; i < argc; i++) {
        in_fp = fopen(argv[i], "r");
        if (in_fp == NULL) {
            fprintf(stderr, "Warning: Could not open '%s'. Skipping.\n", argv[i]);
            continue;
        }

        printf("  Merging: %s\n", argv[i]);
        is_last = (i == (argc - 1));

        while (fgets(line, sizeof(line), in_fp) != NULL) {
            /* 1. Strip all newline characters (CR/LF) */
            len = strlen(line);
            while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')) {
                line[--len] = '\0';
            }
            if (len == 0) continue;
            if (line[0] != ':') continue;

            /* 2. Identify records */
            is_null = (strncmp(line, ":0000000000", 11) == 0);
            is_eof  = (strncmp(line, ":00000001FF", 11) == 0);

            /* 3. Filtering Logic */
            if (is_null || is_eof) {
                if (is_last) {
                    /* Only write if it's the very last file */
                    fprintf(out_fp, "%s\r", line);
                }
                continue;
            }

            /* 4. Write data records */
            fprintf(out_fp, "%s\r", line);
        }
        fclose(in_fp);
    }

    fclose(out_fp);
    printf("Success: Processed %d files.\n", argc - 2);
    printf("Final combined hex file saved as: %s\n", argv[1]);

    return 0;
}
