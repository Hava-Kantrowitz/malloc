#ifndef HMALLOC_H
#define HMALLOC_H

// Husky Malloc Interface
// cs3650 Starter Code

#include <stddef.h>

typedef struct hm_stats {
    long pages_mapped;
    long pages_unmapped;
    long chunks_allocated;
    long chunks_freed;
    long free_length;
} hm_stats;

typedef struct hm_list {
	size_t size;
	struct hm_list* next;
} hm_list;

void printList(hm_list* list); 

hm_stats* hgetstats();
void hprintstats();

void* hmalloc(size_t size);
void hfree(void* item);

#endif
