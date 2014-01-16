#include <stdlib.h>
#include <string.h>

#include "byte-parse.h"

#ifdef DEBUG_MEMORY
#include "mem.h"
#endif /* DEBUG_MEMORY */

#define _cleanup_buffers()    do { \
    memset(ctx->buffer, 0, sizeof(*(ctx->buffer))); \
    ctx->buffer_count = 0; \
    if(ctx->buffer_overflow != NULL) free(ctx->buffer_overflow); \
    ctx->buffer_overflow = NULL; \
    ctx->buffer_overflow_count = 0; \
} while(0)


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

        ctx->buffer_overflow = NULL;
        ctx->buffer_overflow_count = 0;
        _cleanup_buffers();

        ctx->file_pointer = NULL;

        ctx->make_private_record = NULL;
        ctx->priv = NULL;
    }
}

#define _free_field(f)  do { \
    if((f)->content != NULL) { \
        free((f)->content); \
        free((f)); (f) = NULL; \
    } \
} while(0)

static inline void _free_record_content(Record * r)
{
    long int i = 0;

    if(r != NULL) {
        for(i = 0; i < r->field_count; i++) {
            _free_field(r->fields[i]);
            r->fields[i] = NULL;
        }
        free(r->fields);
        r->fields = NULL;
        r->field_count = 0;
    }
}

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
               ctx->fields[i] = NULL; 
            }
            free(ctx->fields);
            ctx->fields = NULL;
        }
        
        if(ctx->records != NULL) {
            for(i = 0; i < ctx->record_count; i++) {
                _free_record_content(ctx->records[i]);
                free(ctx->records[i]);
                ctx->records[i] = NULL;
            }
            free(ctx->records);
            ctx->records = NULL;
        }

        if(ctx->file_pointer != NULL) {
            fclose(ctx->file_pointer);
        }

        byte_init_ctx(ctx);

        print_alloc();
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

Field * _make_new_field(BYTECtx * ctx)
{
    Field * new_field = NULL;
    void * ptr_tmp = NULL;

    new_field = malloc(sizeof(*new_field));
    if(new_field != NULL) {
        if(ctx->copy_values) {
            ptr_tmp = malloc(sizeof(*(ctx->buffer)) *(ctx->buffer_count +
                        ctx->buffer_overflow_count));
            if(ptr_tmp == NULL) {
                free(new_field);
                new_field = NULL;
            } else {
                new_field->content = ptr_tmp;
                memcpy(new_field->content, ctx->buffer_overflow, 
                        sizeof(*(ctx->buffer)) * ctx->buffer_overflow_count);
                memcpy(new_field->content + ctx->buffer_overflow_count,
                        ctx->buffer, sizeof(*(ctx->buffer)) *
                        ctx->buffer_count);
                new_field->content_length = ctx->buffer_count +
                    ctx->buffer_overflow_count;
            }
        } else {
            new_field->content = NULL;
            new_field->content_length = 0;
        }
        
        new_field->real_length = ctx->byte_count - ctx->byte_start;
        new_field->file_position = ctx->byte_start;
        
        ptr_tmp = realloc(ctx->fields, sizeof(*(ctx->fields)) *
                (ctx->field_count + 1));
        if(ptr_tmp == NULL) {
            if(new_field->content != NULL) free(new_field->content);
            free(new_field);
            new_field = NULL;
        } else {
            ctx->fields = ptr_tmp;
            ctx->fields[ctx->field_count] = new_field;
            ctx->field_count++;
        }
    }

    return new_field;
}

ErrorCode _byte_clean_content(BYTECtx * ctx, char * content, long int length)
{
    BYTEType previous = ANY;
    int in_string = 0;
    long int i = 0;
    long int new_length = 0;
    int j = 0;
    
    for(i = 0; i < length; i++) {
        for(j = 0; j < ctx->format_count; j++) {
            if(content[i] == ctx->format[j]->byte) {
                switch(ctx->format[j]->type) {
                    case ESCAPE:
                        if(!in_string) {
                            if(previous != ESCAPE) {
                                new_length++;
                            }
                        }
                        previous = ESCAPE;
                        break;
                    case STRING:
                        if(!in_string && previous == ESCAPE) {
                            content[new_length] = content[i];
                            new_length++;
                        } else {
                            in_string = ! in_string;
                            previous = STRING;
                        }
                        break;
                    case STRING_OPEN:
                        if(!in_string && previous == ESCAPE) {
                            content[new_length] = content[i];
                            new_length;
                        } else {
                            in_string = 1;
                            previous = STRING_OPEN;
                        }
                        break;
                    case STRING_CLOSE:
                        if(!in_string) {
                            content[new_length] = content[i];
                            new_length++;
                        } else {
                            in_string = 0;
                        }
                        break;
                }
            }
        }
    }
}

ErrorCode byte_parse_block(BYTECtx * ctx, const char * block,
        const long int block_length)
{
    Record * new_record = NULL;
    Field * new_field = NULL;
    ErrorCode err = NO_ERROR;
    void * ptr_tmp = NULL;
    long int i = 0;
    int j = 0, done = 0;;

    for(i = 0; i < block_length; i++) {
        
        done = 0;

        if(ctx->previous == FIELD || ctx->previous == RECORD) {
            ctx->byte_start = ctx->byte_count;
        }
        ctx->byte_count++;

        for(j = 0; j < ctx->format_count; j++) {
            if(block[i] == ctx->format[j]->byte) {
                switch(ctx->format[j]->type) {
                    case FIELD:
                        if(ctx->in_string || ctx->previous == ESCAPE) {
                            done = 0; 
                        } else {
                            new_field = _make_new_field(ctx);
                            if(new_field == NULL) {
                                /* i can haz moar memory ? */
                            }

                            done = 1;
                            ctx->previous = FIELD;
                            _cleanup_buffers();
                        }
                        break;
                    case RECORD:
                        if(ctx->in_string || ctx->previous == ESCAPE) {
                            done = 0;
                        } else {
                            new_field = _make_new_field(ctx);
                            if(ctx->fields != NULL) {
                                new_record = malloc(sizeof(*new_record));
                                if(new_record != NULL) {
                                    new_record->fields = ctx->fields;
                                    new_record->field_count = ctx->field_count;
                                        
                                    ctx->fields = NULL;
                                    ctx->field_count = 0;

                                    if(ctx->make_private_record == NULL) {
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
                                    } else {
                                        ctx->make_private_record(new_record,
                                                ctx->priv);
                                        _free_record_content(new_record);
                                        free(new_record);
                                        new_record = NULL;
                                    }
                                }
                            }

                            _cleanup_buffers();
                            ctx->previous = RECORD;
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
                _cleanup_buffers();
                err = MEMORY_ALLOCATION;
            } else {
                ctx->buffer_overflow = ptr_tmp;
                memcpy(ctx->buffer_overflow + ctx->buffer_overflow_count,
                        ctx->buffer, ctx->buffer_count);
                memset(ctx->buffer, 0, BUFFER_SIZE * sizeof(*(ctx->buffer)));
                ctx->buffer_count = 0;
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

ErrorCode byte_load_field_value(BYTECtx * ctx, long int record, long int field)
{
    ErrorCode err = NO_ERROR;
    Field * f = NULL;
    Record * r = NULL;

    if(ctx != NULL && record >= 0 && field >= 0) {
        if(ctx->record_count > record) {
            r = ctx->records[record];
            if(r != NULL && r->field_count > field) {
                f = r->fields[field];
                if(f != NULL) {
                    f->content = malloc(sizeof(*(f->content)) * f->real_length);
                    if(f->content != NULL) {
                        if(fseek(ctx->file_pointer, f->file_position, 
                                    SEEK_SET) != 0) {
                            free(f->content);
                            f->content = NULL;
                            err = POSITION_NOT_AVAILABLE_IN_FILE;
                        } else {
                            if(fread(f->content, sizeof(*(f->content)),
                                   f->real_length, ctx->file_pointer) !=
                                    f->real_length) {
                                free(f->content);
                                f->content = NULL;
                                err = DATA_SIZE_TOO_SMALL;
                            } else {
                                f->content_length = f->real_length;
                            }
                        }
                    }
                }
            }
        } 
    }

    if(err == NO_ERROR && f != NULL && f->content != NULL) {
        /* Clean escape char and all */
    }

    return err;
}

ErrorCode byte_field_to_string(Field * f, char * str, size_t len)
{
    ErrorCode err = GENERIC_ERROR;

    if(f != NULL && str != NULL && len > 1) {
        memset(str, 0, len);
        if(f->content_length < len) {
            memcpy(str, f->content, f->content_length);
        } else {
            memcpy(str, f->content, len - 1);
        }
        err = NO_ERROR;
    }

    return err;
}

ErrorCode byte_register_record_function(BYTECtx * ctx, 
        int (*mpr)(Record * r, void *p), void * priv)
{
    if(ctx != NULL && mpr != NULL) {
        ctx->make_private_record = mpr;
        ctx->priv = priv;
        return NO_ERROR;
    }

    return GENERIC_ERROR;
}
