#include <stdlib.h>
#include <string.h>

#include "csv-parse.h"

#define BUFFER_SIZE         512
#define LINE_FEED           '\n'
#define CARRIAGE_RETURN     '\r'

void csv_free_fields(Field ** fields)
{
    int32_t i = 0;

    if(fields == NULL) return;

    for(i = 0; fields[i] != NULL; i++) {
        if(fields[i]->content != NULL) free(fields[i]->content);
        free(fields[i]);
    }

    free(fields);
}

Field ** csv_parseln(const char separator, const char * line, int32_t len, 
        const int32_t byte, int no_copy)
{
    Field ** fields = NULL;
    Field * new_field = NULL;
    int32_t i = 0, j = 0, buffer_count = 0, field_pos = 0, total_len = 0;
    int32_t start = 0;
    int not_ended = 1;
    int instring = 0;
    char buffer[BUFFER_SIZE] = { '\0' };
    char * tmp = NULL, * temp_str = NULL;

    if(len == -1) len = strlen(line);

    for(i = 0; i < len && not_ended; i++) {

        /* Grow our temp_str when buffer is full the field is not complete */
        if(j >= BUFFER_SIZE) {
            if(! no_copy) {
                buffer_count++;
                tmp = realloc(temp_str, sizeof(*temp_str) * BUFFER_SIZE *
                        buffer_count);
                if(tmp != NULL) {
                    temp_str = tmp;
                    memcpy(temp_str + (sizeof(*temp_str) * BUFFER_SIZE *
                                (buffer_count - 1)), buffer,
                                sizeof(*temp_str) * BUFFER_SIZE);
                } else {
                    csv_free_fields(fields);
                    if(temp_str != NULL) {
                        free(temp_str);
                    }
                    temp_str = NULL;
                    fields = NULL;
                    goto return_fail;
                }
                memset(buffer, '\0', BUFFER_SIZE);
            }
            total_len += j;
            j = 0;
        }

        switch(line[i]) {
            case '"':
                if(i > 0 && line[i - 1] != '\\') {
                    instring = !instring;
                } else {
                    j--;
                    if(!(j >= 0)) j = 0;
                }
                if(! no_copy) buffer[j] = line[i];
                j++;
                break;
            case LINE_FEED:
separator:
                if(instring) {
                    if(! no_copy) buffer[j] = line[i];
                    j++;
                } else {
                    if(i > 0 && line[i] == LINE_FEED &&
                            line[i - 1] == CARRIAGE_RETURN) {
                        if(j > 0) {
                            j--;
                            if(! no_copy) buffer[j] = '\0';
                        }
                    }
make_field:
                    if(!no_copy && j > 0) {
                        buffer_count++;
                        tmp = realloc(temp_str, sizeof(*temp_str) * BUFFER_SIZE *
                                buffer_count);
                        if(tmp != NULL) {
                            temp_str = tmp;
                            memcpy(temp_str + (sizeof(*temp_str) * BUFFER_SIZE *
                                        (buffer_count - 1)), buffer,
                                        sizeof(*temp_str) * BUFFER_SIZE);
                        } else {
                            csv_free_fields(fields);
                            if(temp_str != NULL) {
                                free(temp_str);
                            }
                            temp_str = NULL;
                            fields = NULL;
                            goto return_fail;
                        }
                    }

                    new_field = malloc(sizeof(*new_field));
                    if(new_field != NULL) {

                        if(no_copy) new_field->content = NULL;
                        else new_field->content = temp_str;
                        
                        new_field->len = total_len + j;
                        new_field->pos = field_pos;
                        new_field->at_byte = byte + start;
                        start = i + 1;
                    } else {
                        csv_free_fields(fields);
                        free(temp_str);
                        fields = NULL;
                        goto return_fail;
                    }
                    temp_str = NULL;

                    tmp = realloc(fields, sizeof(*fields) * (field_pos + 2));
                    if(tmp != NULL) {
                        fields = (Field **) tmp;
                        fields[field_pos] = new_field;
                        fields[field_pos + 1] = NULL;
                        field_pos++;
                    } else {
                        csv_free_fields(fields);
                        free(new_field->content);
                        free(new_field);
                        fields = NULL;
                        goto return_fail;
                    }

                    if(! no_copy)
                        memset(buffer, '\0', sizeof(buffer[0]) * BUFFER_SIZE);

                    j = 0;

                    if(line[i] == LINE_FEED) not_ended = 0;
                    buffer_count = 0;
                }
                break;
            default:
                if(line[i] == separator) goto separator; /* harmful, goes up */
                if(! no_copy) buffer[j] = line[i];
                j++;
                break;
        }
    }

    /* We reach the end without a end of line, there's still one field into
       our buffer
     */
    if(not_ended) {
        not_ended = 0;
        goto make_field; /* harmful, goes up */
    }

return_fail:
    return fields;
}
