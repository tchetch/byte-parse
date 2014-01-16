#ifndef MEM_H__
#define MEM_H__

/* Simple allocation traker */

#include <stdlib.h>

struct Alloc {
    void * ptr;

    int m_line;
    int f_line;
    int r_line;

    void * next;
};

struct Alloc * G_ALLOC = NULL;

void * _malloc(size_t size, int line)
{
    struct Alloc * n = NULL;

    n = malloc(sizeof(*n));
    if(n != NULL) {
        n->ptr = malloc(size);
        if(n->ptr != NULL) {
            n->m_line = line;
            n->f_line = -1;
            n->r_line = -1;
            n->next = G_ALLOC;
            G_ALLOC = n;

            return n->ptr;
        }
    }

    return NULL;
}

void * _realloc(void * ptr, size_t size, int line)
{
    struct Alloc * r = NULL;
    void * tmp = NULL;

    if(ptr == NULL) {
        return _malloc(size, line);
    } else {
        for(r = G_ALLOC; r != NULL; r = r->next) {
            if(r->ptr == ptr) {
                tmp = realloc(ptr, size);
                if(tmp != NULL) {
                    r->ptr = tmp;
                    r->r_line = line;
                    return tmp;
                }        
            }
        }
    }

    return NULL;
}

void _free(void * ptr, int line) 
{
    struct Alloc * f = NULL;

    for(f = G_ALLOC; f != NULL; f = f->next) {
        if(f->ptr == ptr) {
            free(f->ptr);
            f->ptr = NULL;
            f->f_line = line;
            break;
        }
    }
}

void print_alloc()
{
    struct Alloc * p = NULL;

    for(p = G_ALLOC; p != NULL; p = p->next) {
        fprintf(stderr, "ALLOC : %8p \t %4d %4d %4d\n", p->ptr,
                p->m_line, p->f_line, p->r_line);
    }
}

#undef malloc
#define malloc(x) _malloc((x), __LINE__)
#undef free
#define free(x) _free((x), __LINE__)
#undef realloc
#define realloc(x,y) _realloc((x), (y), __LINE__)

#endif /* MEM_H__ */
