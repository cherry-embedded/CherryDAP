#include <errno.h>

#include "tlsf.h"
#include "stdint.h"
#include <reent.h>

static tlsf_t tlsf;

__attribute((constructor))
void __pre_malloc_init(void) {
    extern char __heap_start__[],__heap_end__[];
    tlsf = tlsf_create_with_pool(__heap_start__, __heap_end__ - __heap_start__);
}

void *__wrap__malloc_r(struct _reent *reent, size_t size) {
    return tlsf_malloc(tlsf, size);
}

void *__wrap__realloc_r(struct _reent *ptr, void *old, size_t newlen) {
    return tlsf_realloc(tlsf, old, newlen);
}


void __wrap__free_r(struct _reent *reent, void *ptr) {
    return tlsf_free(tlsf, ptr);
}

int getentropy(char buffer[], size_t length)
{
    buffer = buffer; length = length;
    return -ENOSYS;
}
