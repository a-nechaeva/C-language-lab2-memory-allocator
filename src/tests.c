
#include "tests.h"
#include "mem.h"
#include "mem_internals.h"
#include "util.h"

static void * memory_heap;

// Обычное успешное выделение памяти.
static bool test_1() {
    printf("Test 1: Usual successful memory allocation...\n");
    void * malloc_1 = _malloc(100);

    if (malloc_1 == NULL) {
        printf("Test 1 failed :( \n");
        return false;
    }
    printf("Test 1 passed! \n");
    _free(malloc_1);
    return true;
}

// Освобождение одного блока из нескольких выделенных.
static bool test_2() {
    printf("Test 2: Freeing one block from several allocated ones...\n");

    void * malloc_1 = _malloc(100);
    void * malloc_2 = _malloc(200);

    if (malloc_1 == NULL) {
        printf("Test 2 failed: first of two blocks didn't allocate. \n");
        _free(malloc_2);
        return false;
    }

    if (malloc_2 == NULL) {
        printf("Test 2 failed: last of two blocks didn't allocate. \n");
        _free(malloc_1);
        return false;
    }

    if (block_get_header(malloc_1)->is_free || block_get_header(malloc_2)->is_free) {
        printf("Test 2 failed: something went wrong, one or both blocks are empty. \n");
        _free(malloc_1);
        _free(malloc_2);
        return false;
    }

    _free(malloc_1);

    if (!block_get_header(malloc_1)->is_free) {
        printf("Test 2 failed: first block hadn't been freed. \n");
        _free(malloc_2);
        return false;
    }

    if (block_get_header(malloc_1)->is_free && !block_get_header(malloc_2)->is_free) {
        printf("Test 2 passed!\n");
        _free(malloc_2);
        return true;
    }

    if (block_get_header(malloc_1)->is_free && block_get_header(malloc_2)->is_free) {
        printf("Test 2 failed: both blocks had been freed. \n");
        return false;
    }
    printf("Test 2 failed: unexpected error\n");
    return false;
}

// Освобождение двух блоков из нескольких выделенных.
static bool test_3() {
    printf("Test 3: Freeing two blocks from several allocated ones...\n");

    void * malloc_1 = _malloc(100);
    void * malloc_2 = _malloc(150);
    void * malloc_3 = _malloc(200);

    if (malloc_1 == NULL) {
        printf("Test 3 failed: first of three blocks didn't allocate. \n");
        _free(malloc_2);
        _free(malloc_3);
        return false;
    }

    if (malloc_2 == NULL) {
        printf("Test 3 failed: second of three blocks didn't allocate. \n");
        _free(malloc_1);
        _free(malloc_3);
        return false;
    }

    if (malloc_3 == NULL) {
        printf("Test 3 failed: last of three blocks didn't allocate. \n");
        _free(malloc_1);
        _free(malloc_2);
        return false;
    }

    if (block_get_header(malloc_1)->is_free || block_get_header(malloc_2)->is_free || block_get_header(malloc_3)->is_free) {
        printf("Test 3 failed: something went wrong, one or all blocks are empty. \n");
        _free(malloc_1);
        _free(malloc_2);
        _free(malloc_3);
        return false;
    }

    _free(malloc_1);
    _free(malloc_2);

    if (block_get_header(malloc_1)->is_free && block_get_header(malloc_2)->is_free) {
        if (block_get_header(malloc_3)->is_free) {
            printf("Test 3 failed: three blocks had been freed, not two. \n");
            return false;
        } else {
            printf("Test 3 passed! \n");
            _free(malloc_3);
            return true;
        }
    } else {
        printf("Test 3 failed: first and second blocks hadn't been freed. \n");
        _free(malloc_3);
        return false;
    }
}

// Память закончилась, новый регион памяти расширяет старый.
static bool test_4() {
    printf("Test 4: The memory has run out, the new memory region expands the old one...\n");
    void * malloc_1 = _malloc(9000);
    if (malloc_1 == NULL) {
        printf("Test 4 failed: memory didn't allocate. \n");
        return false;
    }

    if (block_get_header(malloc_1)->is_free) {
        printf("Test 4 failed: block is free. \n");
        return false;
    }

    printf("Test 4 passed! \n");
    return true;
}

// Память закончилась, старый регион памяти не расширить из-за другого выделенного диапазона адресов, новый регион выделяется в другом месте.
static bool test_5() {
    printf("Test 5: The memory has run out, the old memory region cannot be expanded due to a different allocated address range, the new region is allocated elsewhere...\n");
    void * malloc_1 = _malloc(3000);

    if (malloc_1 == NULL) {
        printf("Test 5 failed: first block memory didn't allocate. \n");
        return false;
    }
    struct block_header * addr = (struct block_header *) memory_heap;

    map_pages(block_after(addr), 3000, MAP_FIXED);
    
    void * malloc_2 = _malloc(9000);

    if (malloc_2 == NULL) {
        printf("Test 5 failed: second block memory didn't allocate. \n");
        return false;
    }

    if (block_after(addr) != block_get_header(malloc_2)) {
        printf("Test 5 passed! \n");
        _free(malloc_1);
        _free(malloc_2);
        return true;
    }

    printf("Test 5 failed: second block allocated next to the first one. \n");
    _free(malloc_1);
    _free(malloc_2);
    return false;    
}

typedef bool (*tests)();
tests my_tests_array[5] = {test_1, test_2, test_3, test_4, test_5};


void run_tests() {
    memory_heap = heap_init(500);

    if (memory_heap == NULL)
        printf("Error during initialization start memory heap :(");
    else {
        printf("Tests started...\n");
        size_t test_passed = 0;

        for (size_t i = 0; i < 5; i++) {

            if (my_tests_array[i]())
                test_passed++;

        }       

        printf("Passed %zu of 5 tests ^..^ \n", test_passed);
    }

}

