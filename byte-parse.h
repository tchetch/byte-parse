#ifndef BYTE_PARSE_H__
#define BYTE_PARSE_H__

#include <stdio.h>

#ifndef BP_BUFFER_SIZE
#define BP_BUFFER_SIZE     512
#endif 

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
    MEMORY_ALLOCATION,
    FAIL_OPEN_FILE,
    POSITION_NOT_AVAILABLE_IN_FILE,
    DATA_SIZE_TOO_SMALL
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

    /* Temporary storage for fields */
    long int field_count;
    Field ** fields;

    long int record_count;
    Record ** records;

    long int buffer_count;
    char buffer[BP_BUFFER_SIZE];
    
    long int buffer_overflow_count;
    char * buffer_overflow;

    FILE * file_pointer;

    int (*make_private_record)(Record * r, void * priv);
    void * priv;
} BYTECtx;

#define byte_set_no_copy(ctx)   if((ctx) != NULL) { \
    (ctx)->copy_values = 0; \
}

#define byte_unset_no_copy(ctx)   if((ctx) != NULL) { \
    (ctx)->copy_values = 1; \
}

/* Initialize context for parsing */
void byte_init_ctx(BYTECtx * ctx);
/* Reinitialize context for parsing, all data are deleted, file is closed */
void byte_reinit_ctx(BYTECtx * ctx);
/* Add a description. Description tells which byte is an escape char, ending a
   record or a field, ...
 */
ErrorCode byte_add_description(BYTECtx * ctx, const char byte,
        const BYTEType type);
/* Parse a block of data. It doesn't need to have full record or full field,
   juste fread some data and pass to it
 */
ErrorCode byte_parse_block(BYTECtx * ctx, const char * block,
        const long int block_length);
/* Open a file. */
ErrorCode byte_file_open(BYTECtx * ctx, const char * path);
/* DEPRECATED don't use at all */
ErrorCode byte_load_field_value(BYTECtx * ctx, long int record, long int field);
/* Copy a field (field are not null terminated) into a string of given size */
ErrorCode byte_field_to_string(Field * f, char * str, size_t len);
/* Copy a field into a string. Allocate memory and all */
ErrorCode byte_field_to_astring(Field * f, char ** str);
/* Instead of keeping record in byte-parse memory, give a function to be
   used to receive each record once complete. Best way to work with 
   big file (>1G)
 */
ErrorCode byte_register_record_function(BYTECtx * ctx, 
        int (*mpr)(Record * r, void *p), void * priv);

#endif /* BYTE_PARSE_H__ */
