#ifndef BYTE_PARSE_H__
#define BYTE_PARSE_H__

#include <stdio.h>

typedef struct {
    char * content;
    int32_t len;
    int32_t pos;
    int32_t at_byte;
} Field;

typedef struct {
    FILE * fp;
    int32_t record_count;
    int ref_only;
    Field *** records;
} BYTEFile;

void byte_free_fields(Field ** fields);
Field ** byte_parseln(const char separator, const char * line, int32_t len,
        const int32_t byte, int no_copy);
BYTEFile * byte_parse_file(const char * path, const char eol, const char field_sep,
       const int32_t line_len, const int ref_only);
void byte_free_file(BYTEFile * file);

#endif /* BYTE_PARSE_H__ */
