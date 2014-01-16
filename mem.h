#ifndef MEM_H__
#define MEM_H__

/* Simple allocation traker */

#include <stdlib.h>

#define MAX_TAG     10


struct Alloc {
    void * ptr;

    int m_line;
    int f_line;
    int r_line;

    int tag[MAX_TAG];

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
            memset(n->tag, 0, sizeof(n->tag[0]) * MAX_TAG);
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

void tag_ptr(void * ptr, int tag)
{
    struct Alloc * t = NULL;

    for(t = G_ALLOC; t != NULL; t = t->next) {
        if(t->ptr == ptr) {
            if(tag >= MAX_TAG) {
                t->tag[MAX_TAG - 1] = 1;
            } else {
                t->tag[tag] = 1;
            }
            break;
        }
    }
}

void print_alloc()
{
    struct Alloc * p = NULL;
    int i = 0;

    for(p = G_ALLOC; p != NULL; p = p->next) {
        fprintf(stderr, "ALLOC : %8p ", p->ptr);
        fprintf(stderr, "\t %4d %4d %4d\t", p->m_line, p->f_line, p->r_line);
        for(i = 0; i < MAX_TAG; i++) {
            if(p->tag[i] != 0) {
                fprintf(stderr, "[T%2d]", i);
            }
        }
        fprintf(stderr, "\n");
    }
}

#ifndef DEBUG_MEMORY

#undef print_alloc
#define print_alloc()

#define MT0(x)
#define MT1(x)
#define MT2(x)
#define MT3(x)
#define MT4(x)
#define MT5(x)
#define MT6(x)
#define MT7(x)
#define MT8(x)
#define MT9(x)

#else  /* DEBUG_MEMORY */

#undef malloc
#define malloc(x) _malloc((x), __LINE__)
#undef free
#define free(x) _free((x), __LINE__)
#undef realloc
#define realloc(x,y) _realloc((x), (y), __LINE__)

#define MT0(x)    tag_ptr((x), 0)
#define MT1(x)    tag_ptr((x), 1)
#define MT2(x)    tag_ptr((x), 2)
#define MT3(x)    tag_ptr((x), 3)
#define MT4(x)    tag_ptr((x), 4)
#define MT5(x)    tag_ptr((x), 5)
#define MT6(x)    tag_ptr((x), 6)
#define MT7(x)    tag_ptr((x), 7)
#define MT8(x)    tag_ptr((x), 8)
#define MT9(x)    tag_ptr((x), 9)

#endif /* DEBUG_MEMORY */

#endif /* MEM_H__ */
