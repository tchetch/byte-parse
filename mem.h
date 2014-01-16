#ifndef MEM_H__
#define MEM_H__

/* Simple allocation traker */

#define MAX_TAG     10

extern void * G_ALLOC;

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

void * _malloc(size_t size, int line);
void * _realloc(void * ptr, size_t size, int line);
void _free(void * ptr, int line);
void tag_ptr(void * ptr, int tag);
void print_alloc();

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
