#include <stdarg.h>
#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include "mem_internals.h"
#include "mem.h"
#include "util.h"

void debug_block(struct block_header* b, const char* fmt, ... );
void debug(const char* fmt, ... );

extern inline block_size size_from_capacity( block_capacity cap );
extern inline block_capacity capacity_from_size( block_size sz );

static bool            block_is_big_enough( size_t query, struct block_header* block ) { return block->capacity.bytes >= query; }
static size_t          pages_count   ( size_t mem )                      { return mem / getpagesize() + ((mem % getpagesize()) > 0); }
static size_t          round_pages   ( size_t mem )                      { return getpagesize() * pages_count( mem ) ; }

static void block_init( void* restrict addr, block_size block_sz, void* restrict next ) {
  *((struct block_header*)addr) = (struct block_header) {
    .next = next,
    .capacity = capacity_from_size(block_sz),
    .is_free = true
  };
}

static size_t region_actual_size( size_t query ) { return size_max( round_pages( query ), REGION_MIN_SIZE ); }

extern inline bool region_is_invalid( const struct region* r );



void* map_pages(void const* addr, size_t length, int additional_flags) {
  return mmap( (void*) addr, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | additional_flags , 0, 0 );
}

/*  аллоцировать регион памяти и инициализировать его блоком */
static struct region alloc_region  ( void const * addr, size_t query ) {
  //---------------------------------------------------------------------
  if (addr == NULL)
      return REGION_INVALID;

  size_t size = region_actual_size(query);
  void * reg_addr = map_pages(addr, size, MAP_FIXED_NOREPLACE);
  if ((reg_addr == MAP_FAILED) || (reg_addr == NULL))
      return REGION_INVALID;

    block_init(reg_addr, (block_size) {.bytes = size}, NULL);

    return (struct region) {.addr = reg_addr, .size = size, .extends = true};
  //---------------------------------------------------------------------
}

void* block_after( struct block_header const* block )         ;

void* heap_init( size_t initial ) {
  const struct region region = alloc_region( HEAP_START, initial );
  if ( region_is_invalid(&region) ) return NULL;

  return region.addr;
}

#define BLOCK_MIN_CAPACITY 24

/*  --- Разделение блоков (если найденный свободный блок слишком большой )--- */

static bool block_splittable( struct block_header* restrict block, size_t query) {
  return block-> is_free && query + offsetof( struct block_header, contents ) + BLOCK_MIN_CAPACITY <= block->capacity.bytes;
}

static bool split_if_too_big( struct block_header* block, size_t query ) {
  //-------------------------------------------------------------------------
    if (block == NULL)
        return false;

    if (!block_splittable(block, query))
        return false;

    query = size_max(BLOCK_MIN_CAPACITY, query);
    block_size second_block = {.bytes = block->capacity.bytes - query};
    block->capacity.bytes = query;
    void * second_block_add = block_after(block);
    block_init(second_block_add, second_block, block->next);
    block->next = second_block_add;
    return true;
  //--------------------------------------------------------------------------
}


/*  --- Слияние соседних свободных блоков --- */

void* block_after( struct block_header const* block )              {
  return  (void*) (block->contents + block->capacity.bytes);
}
static bool blocks_continuous (
                               struct block_header const* fst,
                               struct block_header const* snd ) {
  return (void*)snd == block_after(fst);
}

static bool mergeable(struct block_header const* restrict fst, struct block_header const* restrict snd) {
  return fst->is_free && snd->is_free && blocks_continuous( fst, snd ) ;
}

static bool try_merge_with_next( struct block_header* block ) {
  //-----------------------------------------------------------------------------
    if (block == NULL)
        return false;

    struct block_header* new_block = block -> next;
    if (new_block == NULL)
        return false;

    if (!mergeable(block, new_block))
        return false;

    block->next = new_block->next;
    block->capacity.bytes = block->capacity.bytes + size_from_capacity(new_block->capacity).bytes;
    return true;
  //-----------------------------------------------------------------------------
}


/*  --- ... ecли размера кучи хватает --- */

struct block_search_result {
  enum {BSR_FOUND_GOOD_BLOCK, BSR_REACHED_END_NOT_FOUND, BSR_CORRUPTED} type;
  struct block_header* block;
};


static struct block_search_result find_good_or_last  ( struct block_header* restrict block, size_t sz )    {
  //-------------------------------------------------------------------------------------------
    if (sz <= 0)
        return (struct block_search_result) {.type = BSR_CORRUPTED};

    sz = size_max(BLOCK_MIN_CAPACITY, sz);
    struct block_header * last_block = NULL;
    struct block_header * current_block = block;

    while (current_block != NULL) {
        if ((current_block->is_free) && (block_is_big_enough(sz, current_block)))
            return (struct block_search_result) {.block = current_block, .type = BSR_FOUND_GOOD_BLOCK};

        last_block = current_block;
        current_block = current_block->next;
    }
    if (last_block == NULL)
        return (struct block_search_result) {.type = BSR_CORRUPTED};

    return (struct block_search_result) {.block = last_block, .type = BSR_REACHED_END_NOT_FOUND};
  //----------------------------------------------------------------------------------------------
}

/*  Попробовать выделить память в куче начиная с блока `block` не пытаясь расширить кучу
 Можно переиспользовать как только кучу расширили. */
static struct block_search_result try_memalloc_existing ( size_t query, struct block_header* block ) {
  //----------------------------------------------------------------------------------
    if (block == NULL)
        return (struct block_search_result) {.type = BSR_CORRUPTED};

    query = size_max(BLOCK_MIN_CAPACITY, query);
    struct block_search_result new_block = find_good_or_last(block, query);

    if (new_block.type == BSR_FOUND_GOOD_BLOCK) {
        split_if_too_big(new_block.block, query);
        new_block.block->is_free = false;
    }

    return new_block;

  //---------------------------------------------------------------------------------
}



static struct block_header* grow_heap( struct block_header* restrict last, size_t query ) {
  //---------------------------------------------------------------------------------
    if (last == NULL)
        return NULL;
    if (BLOCK_MIN_CAPACITY > query)
        query = BLOCK_MIN_CAPACITY;
    void* new_block = block_after(last);
    last->next = alloc_region(new_block, query).addr;

    if (!try_merge_with_next(last))
        return last->next;
    else
        return last;
  //---------------------------------------------------------------------------------
}

/*  Реализует основную логику malloc и возвращает заголовок выделенного блока */
static struct block_header* memalloc( size_t query, struct block_header* heap_start) {
    //-------------------------------------------------------------------
    if (heap_start == NULL)
        return NULL;
    struct block_search_result result = try_memalloc_existing(query, heap_start);
    if (result.type == BSR_REACHED_END_NOT_FOUND) {
        grow_heap(result.block, query);
        result = try_memalloc_existing(query, heap_start);
    }
    if (result.type != BSR_FOUND_GOOD_BLOCK)
        return NULL;
    return result.block;
    //-------------------------------------------------------------------

}

void* _malloc( size_t query ) {
  struct block_header* const addr = memalloc( query, (struct block_header*) HEAP_START );
  if (addr) return addr->contents;
  else return NULL;
}

struct block_header* block_get_header(void* contents) {
  return (struct block_header*) (((uint8_t*)contents)-offsetof(struct block_header, contents));
}

void _free( void* mem ) {
  if (!mem) return ;
  struct block_header* header = block_get_header( mem );
  header->is_free = true;

  //-----------------------------------------------------------
  while(header != NULL) {
      try_merge_with_next(header);
      header = header -> next;
  }
  //-----------------------------------------------------------
}
