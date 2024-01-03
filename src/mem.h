#ifndef _MEM_H_
#define _MEM_H_


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <sys/mman.h>

#define HEAP_START ((void*)0x04040000)

void* _malloc( size_t query );
void  _free( void* mem );
void* heap_init( size_t initial_size );

#define DEBUG_FIRST_BYTES 4

void debug_struct_info( FILE* f, void const* address );
void debug_heap( FILE* f,  void const* ptr );
struct block_header * block_get_header(void * contents);
void* block_after( struct block_header const* block );
void* map_pages(void const* addr, size_t length, int additional_flags);

#endif
