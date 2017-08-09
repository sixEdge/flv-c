#include "util.h"


void *
std_calloc(size_t size)
{
    void *p = malloc(size);
    if (p == NULL) {
        return NULL;
    }
    memset(p, 0, size);
    return p;
}



