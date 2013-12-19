#include <stdlib.h>
#include <string.h>

#include "byte-parse.h"

/* Init context, context must be allocated */
void byte_init_ctx(BYTECtx * ctx)
{
    if(ctx != NULL) {
        ctx->format_count = 0;
        ctx->format = NULL;

        ctx->previous = ANY;
        
        ctx->in_string = 0;
        ctx->copy_values = 1;

        ctx->byte_start = 0;
        ctx->byte_count = 0;
       
        ctx->field_count = 0;
        ctx->fields = NULL;

        ctx->record_count = 0;
        ctx->records = NULL;

        ctx->buffer_count = 0;
        memset(ctx->buffer, 0, sizeof(*(ctx->buffer)) * BUFFER_SIZE);

        ctx->buffer_overflow_count = 0;
        ctx->buffer_overflow = NULL;

        ctx->file_pointer = NULL;
    }
}

#define _free_field(f)  do { \
    if((f)->content != NULL) free((f)->content); \
} while(0)

/* Free allocated memory in ctx */
void byte_reinit_ctx(BYTECtx * ctx)
{
    long int i = 0, j = 0;
    if(ctx != NULL) {
        if(ctx->format != NULL) {
            for(i = 0; i < ctx->format_count; i++) {
                free(ctx->format[i]);         
            }
            free(ctx->format);
        }

        if(ctx->fields != NULL) {
            for(i = 0; i < ctx->field_count; i++) {
                _free_field(ctx->fields[i]); 
            }
            free(ctx->fields);
        }
        
        if(ctx->records != NULL) {
            for(i = 0; i < ctx->record_count; i++) {
                for(j = 0; j < ctx->records[i]->field_count; j++) {
                    _free_field(ctx->records[i]->fields[j]);
                }
                free(ctx->records[i]->fields);
            }
            free(ctx->records);
        }

        if(ctx->file_pointer != NULL) {
            fclose(ctx->file_pointer);
        }

        byte_init_ctx(ctx);
    }
}

/* Add a byte description for parsing
   Return !0 in case of success 0 in case of failure
 */
ErrorCode byte_add_description(BYTECtx * ctx, const char byte, const BYTEType type)
{
    int i = 0;
    void * tmp = NULL;
    BYTEDesc * new_description = NULL;

    if(ctx != NULL) {
        for(i = 0; i < ctx->format_count; i++) {
            if(byte == ctx->format[i]->byte) {
                if( (ctx->format[i]->type == FIELD && type == RECORD) ||
                        (ctx->format[i]->type == RECORD && type == FIELD)) {
                    return SAME_BYTE_FIELD_AND_RECORD;
                }
                if(ctx->format[i]->type == type) {
                    return BYTE_ALREADY_IN_FORMAT;
                }
            }
        }

        new_description = malloc(sizeof(*new_description));
        if(new_description != NULL) {
            new_description->type = type;
            new_description->byte = byte;
            tmp = realloc(ctx->format, sizeof(*(ctx->format)) * 
                    (ctx->format_count + 1));
            if(tmp == NULL) {
                free(new_description);
                return MEMORY_ALLOCATION;
            } else {
                ctx->format = tmp;
                ctx->format[ctx->format_count] = new_description;
                ctx->format_count++;
                return NO_ERROR;
            }
        }
    }

    return GENERIC_ERROR;
}

#define _cleanup_buffers()    do { \
    memset(ctx->buffer, 0, sizeof(*(ctx->buffer))); \
    ctx->buffer_count = 0; \
    free(ctx->buffer_overflow); \
    ctx->buffer_overflow = NULL; \
    ctx->buffer_overflow_count = 0; \
} while(0)

ErrorCode byte_parse_block(BYTECtx * ctx, const char * block,
        const long int block_length)
{
    Record * new_record = NULL;
    Field * new_field = NULL;
    ErrorCode err;
    void * ptr_tmp = NULL;
    long int i = 0;
    int j = 0, done = 0;;

    for(i = 0; i < block_length; i++) {
        
        done = 0;

        for(j = 0; j < ctx->format_count; j++) {
            
            ctx->byte_count++;

            /* We met a record or a field so we are at the begining of a new 
               one 
             */
            if(ctx->previous == FIELD || ctx->previous == RECORD) {
                ctx->byte_start = ctx->byte_count;
            }

            if(block[i] == ctx->format[j]->byte) {
                switch(ctx->format[j]->type) {
                    case FIELD:
                        if(ctx->in_string || ctx->previous == ESCAPE) {
                            done = 0; 
                        } else {
                            new_field = malloc(sizeof(*new_field));
                            if(new_field != NULL) {
                                if(ctx->copy_values) {
                                    ptr_tmp = malloc(sizeof(*(ctx->buffer)) *
                                            (ctx->buffer_count + 
                                             ctx->buffer_overflow_count));
                                    if(ptr_tmp == NULL) {
                                        /* TODO i can haz moar memory ? */
                                    } else {
                                        new_field->content = ptr_tmp;
                                        memcpy(new_field->content, 
                                                ctx->buffer_overflow, 
                                                sizeof(*(ctx->buffer)) *
                                                ctx->buffer_overflow_count);
                                        memcpy(new_field->content + 
                                                ctx->buffer_overflow_count, 
                                                ctx->buffer,
                                                sizeof(*(ctx->buffer)) *
                                                ctx->buffer_count);
                                        new_field->content_length = 
                                            ctx->buffer_count +
                                            ctx->buffer_overflow_count;
                                    }
                                } else {
                                    new_field->content = NULL;               
                                    new_field->content_length = 0;
                                }

                                new_field->real_length =
                                    ctx->byte_count - ctx->byte_start;
                                new_field->file_position = ctx->byte_start;

                                ptr_tmp = realloc(ctx->fields, 
                                        sizeof(*(ctx->fields)) * 
                                        (ctx->field_count + 1));
                                if(ptr_tmp == NULL) {
                                    /* TODO i can haz moar memory ? */
                                } else {
                                    ctx->fields = ptr_tmp;
                                    ctx->fields[ctx->field_count] = new_field;
                                    ctx->field_count++;
                                }
                            }
                            done = 1;
                            _cleanup_buffers();
                        }
                        break;
                    case RECORD:
                        if(ctx->in_string || ctx->previous == ESCAPE) {
                            done = 0;
                        } else {
                            if(ctx->fields != NULL) {
                                new_record = malloc(sizeof(*new_record));
                                if(new_record != NULL) {
                                    new_record->fields = ctx->fields;
                                    new_record->field_count = ctx->field_count;

                                    ctx->fields = NULL;
                                    ctx->field_count = 0;
                                    ptr_tmp = realloc(ctx->records,
                                            sizeof(*(ctx->records)) * 
                                            (ctx->record_count + 1));
                                    if(ptr_tmp == NULL) {
                                        /* TODO i can haz moar memory ? */
                                    } else {
                                        ctx->records = ptr_tmp;
                                        ctx->records[ctx->record_count] =
                                            new_record;
                                        ctx->record_count++;
                                    }
                                }
                            }

                            _cleanup_buffers();
                            done = 1;
                        }
                        break;
                    case ESCAPE:
                        if(ctx->in_string) {
                            done = 0; break;
                        }
                        if(ctx->previous == ESCAPE) {
                            done = 0; 
                        } else {
                            ctx->previous = ESCAPE;
                            done = 1; 
                        }
                        break;
                    case STRING:
                        if(ctx->previous == ESCAPE) {
                            done = 0; 
                        } else {
                            ctx->in_string = !ctx->in_string;
                            done = 1;
                        }
                        break;
                    case STRING_OPEN:
                        if(ctx->in_string || ctx->previous == ESCAPE) { 
                            done = 0;
                        } else {
                            ctx->in_string = 1;
                            done = 1;
                        }
                        break;
                    case STRING_CLOSE:
                        if(! ctx->in_string || ctx->previous == ESCAPE) {
                            done = 0;
                        } else {
                            ctx->in_string = 0;
                            done = 1;
                        }
                        break;
                    default:
                        done = 0;
                        break;
                }
            }
        }

        if(! done) {
            ctx->previous = ANY; /* we set previous as any byte, byte context
                                    doesn't mean anything now
                                  */
            ctx->buffer_count++;
            ctx->buffer[ctx->buffer_count - 1] = block[i];
        }

        /* Now if we have reach our buffer limit, grow buffer_overflow and copy
           the whole buffer into it
         */
        if(ctx->buffer_count >= BUFFER_SIZE) {
            ptr_tmp = realloc(ctx->buffer_overflow, sizeof(*(ctx->buffer)) *
                (ctx->buffer_overflow_count + ctx->buffer_count));
            if(ptr_tmp == NULL) {
                err = MEMORY_ALLOCATION;
            } else {
                ctx->buffer_overflow = ptr_tmp;
                memcpy(ctx->buffer_overflow + ctx->buffer_overflow_count,
                        ctx->buffer, ctx->buffer_count);
                memset(ctx->buffer, 0, BUFFER_SIZE * sizeof(*(ctx->buffer)));
            }
        }
    }

    return err;
}

ErrorCode byte_file_open(BYTECtx * ctx, const char * path)
{
    if(ctx != NULL && path != NULL) {
        ctx->file_pointer = fopen(path, "r");
        if(ctx->file_pointer != NULL) {
            return NO_ERROR;
        } else {
            return FAIL_OPEN_FILE;
        }
    }
    return GENERIC_ERROR;
}
