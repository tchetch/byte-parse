#ifndef BYTE_PARSE_H__
#define BYTE_PARSE_H__

#include <stdio.h>

#define BUFFER_SIZE     512
/* byte-parse API
   --------------

   You should use only byte_parse_file and byte_free_file, they'll call others
   functions as needed.
 */

typedef enum {
    NO_ERROR = 0,
    GENERIC_ERROR,
    BYTE_ALREADY_IN_FORMAT,
    SAME_BYTE_FIELD_AND_RECORD,
    MEMORY_ALLOCATION
} ErrorCode;

typedef struct {
    char * content; 
    long int real_length; 
    long int content_length; 
    
    long int file_position; 
} Field;

typedef struct {
    long int field_count;
    Field ** fields;
} Record;

typedef enum {
    ANY = 0,
    FIELD,
    RECORD,
    ESCAPE,
    STRING, /* When byte is same for open/close */
    STRING_OPEN, 
    STRING_CLOSE
} BYTEType;

typedef struct {
    BYTEType type;
    char byte;
} BYTEDesc;

typedef struct {
    int format_count;
    BYTEDesc ** format;
    
    BYTEType previous;
    
    int in_string;
    int copy_values;

    long int byte_start;
    long int byte_count;

    long int field_count;
    Field ** fields;

    long int record_count;
    Record ** records;

    long int buffer_count;
    char buffer[BUFFER_SIZE];
    
    long int buffer_overflow_count;
    char * buffer_overflow;

    FILE * file_pointer;
} BYTECtx;



#endif /* BYTE_PARSE_H__ */
