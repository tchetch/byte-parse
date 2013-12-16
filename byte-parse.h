#ifndef BYTE_PARSE_H__
#define BYTE_PARSE_H__

#include <stdio.h>

/* byte-parse API
   --------------

   You should use only byte_parse_file and byte_free_file, they'll call others
   functions as needed.
 */
typedef struct {
    char * content; /* Only filled when ref_only == 0 */
    int32_t len; /* Length of content */
    int32_t pos; /* Field position in record */
    int32_t at_byte; /* Byte position in file (useful when ref_only == 1) */
} Field;

typedef struct {
    FILE * fp; /* File pointer */
    int32_t record_count; /* Number of records */
    int ref_only; /* If records contains only reference */
    Field *** records; /* Records */
} BYTEFile;

/* Free a record (array of field), NULL terminated */
void byte_free_fields(Field ** fields);
/* Parse a line either length defined (if len != -1) or separator defined
   (len == -1) and a separator defined */
Field ** byte_parseln(const char separator, const char * line, int32_t len,
        const int32_t byte, int no_copy);
/* Parse a complete file, either with fixed length record (line_len != -1) or
   with a line separator (eol). You need to specify a field separator (field_sep).
   ref_only is when you don't want to copy data from file to memory.

    - path : Path to the file to parse
    - eol : End of line character (char)
    - fiel_sep : End of field separator (char)
    - line_len : If != -1 line will be read as fixed length ignoring eol char
    - ref_only : Don't copy data from file to memory (Field->content == NULL)

   This function leave the file open, it will be close with byte_free_file
 */
BYTEFile * byte_parse_file(const char * path, const char eol, const char field_sep,
       const int32_t line_len, const int ref_only);
/* Close file and free all records and fields */
void byte_free_file(BYTEFile * file);

#endif /* BYTE_PARSE_H__ */
