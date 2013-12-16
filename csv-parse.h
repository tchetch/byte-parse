#ifndef CSV_PARSE_H__
#define CSV_PARSE_H__

typedef struct {
    char * content;
    int32_t len;
    int32_t pos;
    int32_t at_byte;
} Field;

void csv_free_fields(Field ** fields);
Field ** csv_parseln(const char separator, const char * line, int32_t len,
        const int32_t byte, int no_copy);

#endif /* CSV_PARSE_H__ */
