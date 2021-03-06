#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "byte-parse.h"

int main(int argc, char ** argv)
{
    char * value = NULL;
    int x = 0;
    long int i = 0, j = 0;
    long int read_size = 0;
    char buffer[BUFFER_SIZE];
    BYTECtx parser;
    char * files[] = {
        "data/t1.csv",
        "data/t5.tsv",
        NULL
    };

    byte_init_ctx(&parser);
  
    memset(buffer, 0, sizeof(buffer[0]) * BUFFER_SIZE);
    for(x = 0; files[x] != NULL; x++) {
        byte_add_description(&parser, '\n', RECORD);
        byte_add_description(&parser, '\t', FIELD);
        byte_add_description(&parser, '"', STRING);

        if(byte_file_open(&parser, files[x]) == NO_ERROR) {
            while(! feof(parser.file_pointer)) {
                read_size = fread(buffer, sizeof(*buffer), BUFFER_SIZE,
                        parser.file_pointer);
                if(read_size > 0) {
                    if(byte_parse_block(&parser, buffer, BUFFER_SIZE) != NO_ERROR) {
                        fprintf(stderr, "What's that sound\n");
                    } 
                } else {
                    break;
                }
            }
        }

        for(i = 0; i < parser.record_count; i++) {
            for(j = 0; j < parser.records[i]->field_count; j++) {
                if(byte_field_to_astring(parser.records[i]->fields[j], &value)
                       == NO_ERROR) {
                    printf("<%s>\t", value);
                    free(value);
                }
            }
            printf("\n");
        }

        byte_reinit_ctx(&parser);
    }

    return 0;    
}

