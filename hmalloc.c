
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>

#include "hmalloc.h"

/*
  typedef struct hm_stats {
  long pages_mapped;
  long pages_unmapped;
  long chunks_allocated;
  long chunks_freed;
  long free_length;
  } hm_stats;
*/

const size_t PAGE_SIZE = 4096;
static hm_stats stats; // This initializes the stats to 0.
static hm_list* free_alist = NULL;//this is my free list  

long
free_list_length()
{
    long length = 0;
    //printf("got here to list length calc\n");

    hm_list* curr = free_alist;
    //printf("At the free list length, length is "); 
    //printList(curr); 
    while (curr != 0) {
	   //printf("made it to a\n"); 
	   length++;
	   //printf("length is %d\n", length); 
	   if (length == 2) {
		   break;
	   }
	   curr = curr->next;
	   //printf("made it to b\n");  
    } 
    //printf("calculated length\n"); 
    return length;
}

hm_stats*
hgetstats()
{
    stats.free_length = free_list_length();
    //stats.free_length = 2; 
    return &stats;
}

void
hprintstats()
{
    stats.free_length = free_list_length();
    //stats.free_length = 2; 
    fprintf(stderr, "\n== husky malloc stats ==\n");
    fprintf(stderr, "Mapped:   %ld\n", stats.pages_mapped);
    fprintf(stderr, "Unmapped: %ld\n", stats.pages_unmapped);
    fprintf(stderr, "Allocs:   %ld\n", stats.chunks_allocated);
    fprintf(stderr, "Frees:    %ld\n", stats.chunks_freed);
    fprintf(stderr, "Freelen:  %ld\n", stats.free_length);
}

static
size_t
div_up(size_t xx, size_t yy)
{
    // This is useful to calculate # of pages
    // for large allocations.
    size_t zz = xx / yy;

    if (zz * yy == xx) {
        return zz;
    }
    else {
        return zz + 1;
    }
}

void
printList(hm_list* list) {
	int i = 0;
	for (; list; list = list->next) {
		int list_size = (int) list->size;
		void* list_pointer = (void*) list->next;
		printf("Node %d has val %d, next %p\n", i, list_size, list_pointer);
		i++;
	}
}

void
insertFree(hm_list* node) {
	//if there is nothing in the list just put the node right there
	if (free_alist == 0) {
		free_alist = node;
		//printf("list was empty\n"); 
	        return; 	
	}
	
	//otherwise we have to loop through to see where it goes
	//this is the insert
	/*hm_list* curr = free_alist; //the head of list
	hm_list* prev = 0; 
	while (curr != 0) {
		//if the current address is greater than the given address it has to be the previous one, replace previous
		if ((void*) curr > (void*) node) {
			//join hands
			//want to see if the address before is equal to the insert address
			size_t prev_size = 0; 
			if (prev != 0) {
				prev_size = prev->size; 
			}
			void* prev_address = (void*) prev + prev_size;
			void* insert_addr_start = (void*) node;

			//or if the insert address is equal to its next address
			size_t node_size = node->size;
			void* insert_addr_end = (void*) node + node_size; 
			void* next_address = (void*) curr; 

			//coalesce both sides, prev/insert and insert/next
			if (prev_address == insert_addr_start && insert_addr_end == next_address) {
				//make it all part of the prev block
				prev->size = prev_size + node_size + curr->size; 
				prev->next = curr->next; 
				printf("got to coalesce both\n"); 
			}
			//coalesce prev and insert
			else if (prev_address == insert_addr_start) {
				//make both part of prev block
				prev->size = prev_size + node_size;
				prev->next = curr; 
				printf("got to prev\n"); 
			}
			//coalesce insert and next 
			else if (insert_addr_end == next_address) {
				//insert node and remove curr, keeping size with insert
				node->size = node_size + curr->size;
			        if (prev != 0) {	
					prev->next = node; 
				}
				node->next = curr->next; 
				printf("got to next\n"); 
			}
					
			//no coalesce
			else {
				if (prev != 0) {
					prev->next = node;
				}
				node->next = curr;
			        printf("no coalesce\n"); 	
			}

		        //want to break out of loop
			//printf("breaking from loop\n");
			if (prev == 0) {
				free_alist = node;
			}
			break;  
		}

		prev = curr; 
		curr = curr->next;

	}*/

}

void*
hmalloc(size_t size)
{
 
    stats.chunks_allocated += 1;
    size += sizeof(size_t);
    //first check to see if there is a big enough block within the free list
    //if there isn't, mmap
    hm_list* block = 0;
    hm_list* curr = free_alist; //this is the head of the list
    hm_list* prev = 0; 
    if (size <= PAGE_SIZE) {
    for (; free_alist; free_alist = free_alist->next) {
	    if (free_alist->size >= size) {
		    block = free_alist; 
		    //if it's at the head set prev next to curr next
		    //otherwise just set the head of list
		    if (prev != 0) {
			   prev->next = curr->next;
		    }
		    else {
			   free_alist = curr->next;
		    }
		    //this returns when we have enough space in the free list
		    //check if leftover has room to hold node
		    if (block->size - size >= sizeof(hm_list)) {
			   //put extra into free list
			   //get address
			   void* extra = (void*) block + size; 
			   hm_list* extra_block = (hm_list*) extra; 
			   extra_block->size = block->size - size;
			   extra_block->next = 0;

			   //printf("print list before insertion is ");
			   //printList(free_alist);
			   insertFree(extra_block);
			   //printf("print list after insertion is "); 
			   //printList(free_alist);  

			   //block is now just the size
			   block->size = size; 
		    } 
		    return (void*) block + sizeof(size_t);
	    }

	    prev = curr;
	    curr = curr->next; 
    }

    //mmap block this is when it's less than a page but we don't have space in list
    block = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
    block->size = PAGE_SIZE; 
    block->next = NULL;
    stats.pages_mapped++;
      
    if (block->size - size >= sizeof(hm_list)) {
	    //add extra to free list
	    //find address of the extra
	    void* extra = (void*) block + size;
	    //create node at that address
	    hm_list* extra_block = (hm_list*) extra;
	    extra_block->size = block->size - size;
	    extra_block->next = 0;  

	    //printf("free list before insertion is ");
	    //printList(free_alist); 
	    insertFree(extra_block); 
	    //printf("free list after insertion is ");
	    //printList(free_alist); 
	    
	    //block size is now just the size
	    block->size = size; 
    }

    // return pointer after size field
    return (void*) block + sizeof(size_t);

    }

    else {
	    int numPages = (size + PAGE_SIZE - 1) / PAGE_SIZE; 
	    stats.pages_mapped += numPages;
	    size_t bigSize = PAGE_SIZE * numPages;
	    block = mmap(NULL, bigSize, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0); 
	    block->size = bigSize;
	    block->next = NULL; 
	    return (void*) block + sizeof(size_t); 
    }
}

void
hfree(void* item)
{
    stats.chunks_freed += 1;

    //Actually free the item.

    hm_list* node = (hm_list*) (item - sizeof(size_t));  

    if (node->size < PAGE_SIZE) {

	    //printf("The free list before insertion is ");
	    //printList(free_alist); 
	    insertFree(node);  
	    //printf("The free list after insertion is ");
	    //printList(free_alist); 
    }

    if (node->size >= PAGE_SIZE) {
	    //find number of pages to munmap
	    int firstHalf = node->size + PAGE_SIZE - 1;
	    int totalPages = firstHalf/PAGE_SIZE; 
	    munmap((void*)node, node->size);
	    stats.pages_unmapped+= totalPages; 
    } 
}

