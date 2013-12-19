#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "byte-parse.h"

int main(int argc, char ** argv)
{
    char * value = NULL;
    long int i = 0, j = 0;
    long int read_size = 0;
    char buffer[BUFFER_SIZE];
    BYTECtx parser;

    byte_init_ctx(&parser);
    
    byte_add_description(&parser, '\n', RECORD);
    byte_add_description(&parser, '\t', FIELD);

    if(byte_file_open(&parser, "data/t1.csv") == NO_ERROR) {
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
            value = malloc((parser.records[i]->fields[j]->content_length + 1) *
                   sizeof(*(parser.records[i]->fields[j]->content)));
            if(value != NULL) {
                memset(value, '\0',
                        (parser.records[i]->fields[j]->content_length + 1) *
                        sizeof(*(parser.records[i]->fields[j]->content)));
                memcpy(value, parser.records[i]->fields[j]->content,
                        parser.records[i]->fields[j]->content_length *
                        sizeof(*(parser.records[i]->fields[j]->content)));
                printf("<%s> \t", value);
                free(value);
            }
        }
        printf("\n");
    }

    return 0;    
}

