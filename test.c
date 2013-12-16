#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "byte-parse.h"

int main(int argc, char ** argv)
{
    Field ** current = NULL;
    int32_t i = 0, j = 0;
    BYTEFile * f = NULL;
    char * val = NULL;

    printf("*** ASCII File test ***\n");
    f = byte_parse_file("data/t1.csv", '\n', '\t', -1, 0);
    if(f != NULL) {
        for(i = 0; f->records[i] != NULL; i++) {
            current = f->records[i];
            for(j = 0; current != NULL && current[j] != NULL; j++) {
                val = malloc((current[j]->len + 1) * sizeof(*val));
                if(val != NULL) {
                    memset(val, '\0', current[j]->len + 1);
                    memcpy(val, current[j]->content, current[j]->len);
                    printf("| VAL : \"%s\" |", val);
                    free(val);
                }
            }
            printf("\n");
        }
    }
    byte_free_file(f);

    printf("*** Binary File test ***\n");
    f = byte_parse_file("data/t2.dat", 0x03, 0x00, -1, 0);
    if(f != NULL) {
        for(i = 0; f->records[i] != NULL; i++) {
            current = f->records[i];
            for(j = 0; current != NULL && current[j] != NULL; j++) {
                val = malloc((current[j]->len + 1) * sizeof(*val));
                if(val != NULL) {
                    memset(val, '\0', current[j]->len + 1);
                    memcpy(val, current[j]->content, current[j]->len);
                    printf("| VAL : \"%s\" |", val);
                    free(val);
                }
            }
            printf("\n");
        }
    }
    byte_free_file(f);
    
    printf("*** Binary File test, fixed length ***\n");
    f = byte_parse_file("data/t3.dat", 0x00, 0x00, 11, 0);
    if(f != NULL) {
        for(i = 0; f->records[i] != NULL; i++) {
            current = f->records[i];
            for(j = 0; current != NULL && current[j] != NULL; j++) {
                val = malloc((current[j]->len + 1) * sizeof(*val));
                if(val != NULL) {
                    memset(val, '\0', current[j]->len + 1);
                    memcpy(val, current[j]->content, current[j]->len);
                    printf("| VAL : \"%s\" |", val);
                    free(val);
                }
            }
            printf("\n");
        }
    }
    byte_free_file(f);

    return 0;    
}

