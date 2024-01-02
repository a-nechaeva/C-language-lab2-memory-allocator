
#include "tests.h"
#include "mem.h"
#include "mem_internals.h"

// Обычное успешное выделение памяти.
bool test_1(void * start_heap) {
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
bool test_2(void * start_heap) {
    printf("Test 2: Freeing one block from several allocated ones...\n");

    void * malloc_1 = _malloc(100);
    void * malloc_2 = _malloc(100);

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
bool test_3(void * start_heap) {
    printf("Test 3: Freeing two blocks from several allocated ones...\n");

    void * malloc_1 = _malloc(100);
    void * malloc_2 = _malloc(100);
    void * malloc_3 = _malloc(100);

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
bool test_4(void * start_heap) {

}

// Память закончилась, старый регион памяти не расширить из-за другого выделенного диапазона адресов, новый регион выделяется в другом месте.
bool test_5(void * start_heap) {

}

void run_tests() {
    void * memory_heap = heap_init(2 * REGION_MIN_SIZE);

    if (memory_heap == NULL)
        printf("Error during initialization start memory heap :(");
    else {
        printf("Tests started...\n");
        size_t test_passed = 0;

        if (test_1(memory_heap))
            test_passed += 1;
        if (test_2(memory_heap))
            test_passed += 1;
        if (test_3(memory_heap))
            test_passed += 1;
        if (test_4(memory_heap))
            test_passed += 1;
        if (test_5(memory_heap))
            test_passed += 1;

        printf("Passed %Iu of 5 tests.", test_passed);
    }

}

