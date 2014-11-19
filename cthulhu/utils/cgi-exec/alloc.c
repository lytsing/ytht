/* alloc.c -- simple area allocator -
   Feb. 2003 Thomas Ogrisegg
 */
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define die2(x) { perror (x); exit (23); }
#define PAGE_SIZE 0x1000
#define PAGE_ALIGN(x) (((x) + PAGE_SIZE-1) & ~(PAGE_SIZE-1))

#ifndef MAP_ANON
# ifdef MAP_ANONYMOUS
#  define MAP_ANON MAP_ANONYMOUS
# endif
#endif

typedef struct __alloc {
	void *base;
	size_t len;
	size_t plen;
	struct __alloc *next;
} alloc_t;

static alloc_t *base;
static alloc_t *cbase;

#ifdef MAP_ANON
void *
alloc_page (size)
{
	return (mmap (NULL, size, PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE, -1, 0));
}
#else
void *
alloc_page (size)
{
	return (malloc (size));
}
#endif

alloc_t *
init_alloc (size)
{
	alloc_t *res = alloc_page (size);
	if (!res || res == (void *) -1)
		die2 ("Out of memory");
	res->base = res+1;
	res->len  = 0;
	res->plen = size;
	res->next = NULL;
	return (res);
}

void *
alloc (size, base)
	size_t size;
	alloc_t **base;
{
	alloc_t *x;
	if (!*base) *base = init_alloc (PAGE_SIZE);
	x = *base;
	while ((x->len+size) > x->plen) {
		if (!x->next) x->next =
			init_alloc (PAGE_ALIGN (size+sizeof (alloc_t)));
		x = x->next;
	}
	x->len += size;
	return ((void *)((long)x->base+x->len-size));
}

void *
cxalloc (size)
	size_t size;
{
	return (alloc (size, &cbase));
}

void *
xxalloc (size)
	size_t size;
{
	return (alloc (size, &base));
}

void
alloc_reset ()
{
	alloc_t *x = base;
	while (x) { x->len = 0; x = x->next; }
}

void
config_alloc_reset ()
{
	alloc_t *x = cbase;
	while (x) { x->len = 0; x = x->next; }
}
