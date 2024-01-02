
#include "tests.h"
#include "mem.h"
#include "mem_internals.h"

// Обычное успешное выделение памяти.
bool test_1(void * start_heap) {
    printf("Test 1: Usual successful memory allocation...\n");
    void * malloc_1 = _malloc(1000);

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

}

// Освобождение двух блоков из нескольких выделенных.
bool test_3(void * start_heap) {

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

