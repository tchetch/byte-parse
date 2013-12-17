#include <stdlib.h>
#include <string.h>

#include "byte-parse.h"

#define BUFFER_SIZE         512
#define LINE_FEED           '\n'
#define CARRIAGE_RETURN     '\r'

void byte_free_fields(Field ** fields)
{
    int32_t i = 0;

    if(fields == NULL) return;

    for(i = 0; fields[i] != NULL; i++) {
        if(fields[i]->content != NULL) free(fields[i]->content);
        free(fields[i]);
    }

    free(fields);
}

Field ** byte_parseln(const char separator, const char * line, int32_t len, 
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
                    byte_free_fields(fields);
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
                            byte_free_fields(fields);
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
                        byte_free_fields(fields);
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
                        byte_free_fields(fields);
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

struct Line {
    char * content;
    int32_t len;
};

struct Line * _byte_read_fixed_line_len(FILE * fp, int32_t line_len)
{
    struct Line * l = NULL;
    int32_t lilen = 0;
    char * line = NULL;

    line = malloc(line_len * sizeof(char));
    if(line != NULL) {
        lilen = fread(line, sizeof(char), line_len, fp);
        if(lilen == line_len) {
            l = malloc(sizeof(*l));
            if(l != NULL) {
                l->content = line;
                l->len = lilen;    
            }
        }
    }

    return l;
}

struct SRead {
    int32_t len;
    char * content;
};

struct Line * _byte_read_variable_line_len(FILE * fp, const char eol, 
        void ** saved_read)
{
    struct Line * l = NULL;
    int32_t lilen = 0, i = 0;
    char * line = NULL;
    int found = 0, eof = 0, err = 0;
    char buffer[BUFFER_SIZE] = { '\0' };
    void * tmp = NULL;
    int buffer_count = 0;
    long file_pos = 0;
    struct SRead * sread = NULL;

    if(saved_read == NULL) return NULL;

    do {
        if(*saved_read != NULL) {
            sread = (struct SRead *)*saved_read;
            lilen = sread->len;
            memcpy(buffer, sread->content, sread->len);
            free(sread);
            *saved_read = NULL;
        } else {
            lilen = fread(buffer, sizeof(char), BUFFER_SIZE, fp);
        }
        if(lilen > 0) {
            for(i = 0; i < BUFFER_SIZE; i++) {
                if(buffer[i] == eol) {
                    found = 1;
                    break;
                }
            }
            
            tmp = realloc(line, sizeof(char) *
                    ((BUFFER_SIZE * buffer_count) + i + 1));
            if(tmp != NULL) {
                line = tmp;
                memcpy(line + (BUFFER_SIZE * buffer_count), buffer, i + 1);
            
                if(feof(fp) || lilen < BUFFER_SIZE) eof = 1;
           
                if(found) {
                    l = malloc(sizeof(*l));
                    if(l != NULL) {
                        l->content = line;
                        l->len = (BUFFER_SIZE * buffer_count) + i;
                    }
                    if(i < BUFFER_SIZE) {
                        sread = malloc(sizeof(*sread) + (BUFFER_SIZE - i));
                        if(sread != NULL) {
                            sread->content = ((char *)sread) + sizeof(*sread);
                            sread->len = BUFFER_SIZE - i;
                            memcpy(sread->content, &buffer[i+1], 
                                    BUFFER_SIZE - i - 1);
                            *saved_read = sread;
                        }
                    }
                }

                buffer_count++;
            } else {
                err = 1;
            }
        } else {
            err = 1;
        }
    } while(!found && !eof && !err);

    return l;
}

BYTEFile * byte_parse_file(const char * path, const char eol, const char field_sep,
       const int32_t line_len, const int ref_only)
{
    BYTEFile * file = NULL;
    Field ** l_fields = NULL;
    struct Line * line = NULL;
    FILE * fp = NULL;
    int32_t byte_count = 0;
    void * tmp = NULL;
    int eof = 0;
    void * saved_read = NULL;

    fp = fopen(path, "r"); /* Not closed at the end */
    if(fp != NULL) {
        file = malloc(sizeof(*file));
        if(file != NULL) {
            file->fp = fp;
            file->record_count = 0;
            file->ref_only = ref_only;
            file->records = NULL;

            while(!eof) {
                if(line_len == -1) {
                    line = _byte_read_variable_line_len(fp, eol, &saved_read);
                    if(line == NULL) {
                        eof = 1;
                        break;
                    }
                } else {
                    line = _byte_read_fixed_line_len(fp, line_len);
                    if(line == NULL) {
                        eof = 1;
                        break;
                    }
                }
        
                if(line->content != NULL && line->len > 0) {
                    l_fields = byte_parseln(field_sep, line->content, line->len,
                            byte_count, ref_only);
                    byte_count += line->len;
                    if(l_fields != NULL) {
                        tmp = realloc(file->records, sizeof(*(file->records)) *
                               (file->record_count + 2));
                        if(tmp != NULL) {
                            file->records = (Field ***)tmp;
                            file->records[file->record_count] = l_fields;
                            file->records[file->record_count + 1] = NULL;
                            file->record_count++;
                        } 
                    }
                
                    free(line->content);
                    free(line);
                    line = NULL;
                }
            }
        }
    }

    return file;
}

void byte_free_file(BYTEFile * file)
{
    int32_t i = 0;

    if(file != NULL) {
        if(file->fp != NULL) fclose(file->fp);

        if(file->records != NULL) {
            for(i = 0; file->records[i] != NULL; i++) {
                byte_free_fields(file->records[i]);
            }
            free(file->records);
        }
        free(file);
    }
}
