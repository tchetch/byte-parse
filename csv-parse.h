#ifndef CSV_PARSE_H__
#define CSV_PARSE_H__

typedef struct {
    char * content;
    int32_t len;
    int32_t pos;
} Field;

void csv_free_fields(Field ** fields);
Field ** csv_parseln(const char separator, const char * line, int32_t len);

#endif /* CSV_PARSE_H__ */
