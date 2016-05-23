/*
 * Copyright (c) 2015 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "CppUTest/TestHarness.h"
#include "nsdynmemLIB.h"
#include <stdlib.h>
#include <stdio.h>
#include "error_callback.h"

TEST_GROUP(dynmem)
{
    void setup() {
        reset_heap_error();
    }

    void teardown() {
    }
};


// Test just normal init, with size of 1kB
TEST(dynmem, init)
{
    uint16_t size = 1000;
    uint8_t *heap = (uint8_t*)malloc(size);
    CHECK(NULL != heap);
    mem_stat_t info;
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
    CHECK(info.heap_sector_size >= (size-4)); // Allow 4 bytes of alignment to happend
    CHECK(!heap_have_failed());
    CHECK(ns_dyn_mem_get_mem_stat() == &info);
    free(heap);
}

// Test init with sizes between 1kB and 32kB
TEST(dynmem, different_sizes)
{
    reset_heap_error();
    for (uint16_t size = 1000; size<32768; size++) {
        mem_stat_t info;
        uint8_t *heap = (uint8_t*)malloc(size);
        ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
        CHECK(info.heap_sector_size >= (size-4)); // Allow 4 bytes of alignment to happend
        CHECK(!heap_have_failed());
        CHECK(ns_dyn_mem_alloc(10)); // test allocatio works
        free(heap);
    }
}

// Test with different alignment
TEST(dynmem, diff_alignment)
{
    uint16_t size = 1000;
    mem_stat_t info;
    uint8_t *heap = (uint8_t*)malloc(size);
    uint8_t *ptr = heap;
    CHECK(NULL != heap);
    reset_heap_error();
    for (int i=0; i<16; i++) {
        ptr++; size--;
        ns_dyn_mem_init(ptr, size, &heap_fail_callback, &info);
        CHECK(info.heap_sector_size >= (size-4)); // Allow 4 bytes of alignment to happend
        CHECK(!heap_have_failed());
    }
    free(heap);
}

// Allocate until full, then free everything
TEST(dynmem, ns_dyn_mem_alloc)
{
    uint16_t size = 1000;
    mem_stat_t info;
    void *p[size];
    uint8_t *heap = (uint8_t*)malloc(size);
    CHECK(NULL != heap);
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
    CHECK(!heap_have_failed());
    int block = 1;

    int i;
    for (i=0; i<size; i++) {
        p[i] = ns_dyn_mem_alloc(block);
        if (!p[i])
            break;
    }
    CHECK(!heap_have_failed()); // Mem full is not failure
    CHECK(info.heap_alloc_fail_cnt == 1); // Should have failed
    CHECK(info.heap_sector_alloc_cnt == i); // Number of allocations should match the number of loops until failure
    CHECK(info.heap_sector_allocated_bytes == info.heap_sector_allocated_bytes_max); // We should now be at max level

    for (; i>=0; i--) {
        ns_dyn_mem_free(p[i]);
    }
    CHECK(!heap_have_failed());
    CHECK(info.heap_sector_alloc_cnt == 0);
    free(heap);
}

// Allocate until full, then free everything
TEST(dynmem, ns_dyn_mem_temporary_alloc)
{
    uint16_t size = 1000;
    mem_stat_t info;
    void *p[size];
    uint8_t *heap = (uint8_t*)malloc(size);
    CHECK(NULL != heap);
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
    CHECK(!heap_have_failed());
    int block = 1;

    int i;
    for (i=0; i<size; i++) {
        p[i] = ns_dyn_mem_temporary_alloc(block);
        if (!p[i])
            break;
    }
    CHECK(!heap_have_failed()); // Mem full is not failure
    CHECK(info.heap_alloc_fail_cnt == 1); // Should have failed
    CHECK(info.heap_sector_alloc_cnt == i); // Number of allocations should match the number of loops until failure
    CHECK(info.heap_sector_allocated_bytes == info.heap_sector_allocated_bytes_max); // We should now be at max level

    for (; i>=0; i--) {
        ns_dyn_mem_free(p[i]);
    }
    CHECK(!heap_have_failed());
    CHECK(info.heap_sector_alloc_cnt == 0);
    free(heap);
}

TEST(dynmem, test_both_allocs_with_hole_usage) {
    uint16_t size = 48;
    mem_stat_t info;
    void *p[size];
    uint8_t *heap = (uint8_t*)malloc(size);
    CHECK(NULL != heap);
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
    CHECK(!heap_have_failed());

    void *ptr = ns_dyn_mem_alloc(15);
    void *ptr2 = ns_dyn_mem_alloc(4);

    ns_dyn_mem_free(ptr);
    ns_dyn_mem_free(ptr2);
    CHECK(info.heap_sector_allocated_bytes == 0);

    void *ptr3 = ns_dyn_mem_temporary_alloc(15);
    void *ptr4 = ns_dyn_mem_temporary_alloc(5);

    ns_dyn_mem_free(ptr3);
    ns_dyn_mem_free(ptr4);
    CHECK(info.heap_sector_allocated_bytes == 0);

    free(heap);
}

TEST(dynmem, zero_allocate)
{
    uint16_t size = 1000;
    mem_stat_t info;
    uint8_t *heap = (uint8_t*)malloc(size);
    uint8_t *ptr = heap;
    CHECK(NULL != heap);
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
    CHECK(!heap_have_failed());
    ns_dyn_mem_alloc(0);
    CHECK(heap_have_failed());
    CHECK(NS_DYN_MEM_ALLOCATE_SIZE_NOT_VALID == current_heap_error);
    free(heap);
}

TEST(dynmem, too_big)
{
    uint16_t size = 1000;
    mem_stat_t info;
    uint8_t *heap = (uint8_t*)malloc(size);
    uint8_t *ptr = heap;
    CHECK(NULL != heap);
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
    CHECK(!heap_have_failed());
    ns_dyn_mem_alloc(size);
    CHECK(heap_have_failed());
    CHECK(NS_DYN_MEM_ALLOCATE_SIZE_NOT_VALID == current_heap_error);
    free(heap);
}

TEST(dynmem, diff_sizes)
{
    uint16_t size = 1000;
    mem_stat_t info;
    void *p;
    uint8_t *heap = (uint8_t*)malloc(size);
    CHECK(NULL != heap);
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
    CHECK(!heap_have_failed());
    int i;
    // Should leave headroom for 2 pointers
    for (i=1; i<(size-8); i++) {
        p = ns_dyn_mem_temporary_alloc(i);
        CHECK(p);
        ns_dyn_mem_free(p);
        CHECK(!heap_have_failed());
    }
    CHECK(!heap_have_failed()); // Mem full is not failure
    CHECK(info.heap_sector_alloc_cnt == 0);
    free(heap);
}

TEST(dynmem, double_free)
{
    uint16_t size = 1000;
    mem_stat_t info;
    uint8_t *heap = (uint8_t*)malloc(size);
    void *p;
    CHECK(NULL != heap);
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
    CHECK(!heap_have_failed());
    p = ns_dyn_mem_alloc(100);
    CHECK(p);
    ns_dyn_mem_free(p);
    CHECK(!heap_have_failed());
    ns_dyn_mem_free(p);
    CHECK(heap_have_failed());
    CHECK(NS_DYN_MEM_DOUBLE_FREE == current_heap_error);
    free(heap);
}

TEST(dynmem, middle_free)
{
    uint16_t size = 1000;
    mem_stat_t info;
    uint8_t *heap = (uint8_t*)malloc(size);
    void *p[3];
    CHECK(NULL != heap);
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
    CHECK(!heap_have_failed());
    for (int i=0; i<3; i++) {
        p[i] = ns_dyn_mem_alloc(100);
        CHECK(p);
    }
    ns_dyn_mem_free(p[1]);
    CHECK(!heap_have_failed());
    ns_dyn_mem_free(p[0]);
    CHECK(!heap_have_failed());
    ns_dyn_mem_free(p[2]);
    CHECK(!heap_have_failed());
    free(heap);
}

TEST(dynmem, over_by_one)
{
    uint16_t size = 1000;
    mem_stat_t info;
    uint8_t *heap = (uint8_t*)malloc(size);
    uint8_t *p;
    CHECK(NULL != heap);
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
    CHECK(!heap_have_failed());
    p = (uint8_t *)ns_dyn_mem_alloc(100);
    CHECK(p);
    p[100] = 0xff; //Write one over the reserved size
    ns_dyn_mem_free(p);
    CHECK(heap_have_failed());
    CHECK(NS_DYN_MEM_HEAP_SECTOR_CORRUPTED == current_heap_error);
    free(heap);
}

TEST(dynmem, not_from_this_heap)
{
    uint16_t size = 1000;
    mem_stat_t info;
    uint8_t *heap = (uint8_t*)malloc(size);
    uint8_t *p;
    CHECK(NULL != heap);
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
    CHECK(!heap_have_failed());
    p = (uint8_t *)ns_dyn_mem_alloc(100);
    CHECK(p);
    ns_dyn_mem_free(&heap[-1]);
    CHECK(heap_have_failed());
    CHECK(NS_DYN_MEM_POINTER_NOT_VALID == current_heap_error);
    reset_heap_error();
    ns_dyn_mem_free(&heap[1001]);
    CHECK(heap_have_failed());
    CHECK(NS_DYN_MEM_POINTER_NOT_VALID == current_heap_error);
    free(heap);
}

TEST(dynmem, free_on_empty_heap)
{
    uint16_t size = 1000;
    mem_stat_t info;
    uint8_t *heap = (uint8_t*)malloc(size);
    uint8_t *p;
    CHECK(NULL != heap);
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
    CHECK(!heap_have_failed());
    ns_dyn_mem_free(&heap[1]);
    CHECK(heap_have_failed());
    CHECK(NS_DYN_MEM_POINTER_NOT_VALID == current_heap_error);
    free(heap);
}

TEST(dynmem, not_negative_stats)
{
    uint16_t size = 1000;
    mem_stat_t info;
    uint8_t *heap = (uint8_t*)malloc(size);
    void *p;
    CHECK(NULL != heap);
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, &info);
    CHECK(!heap_have_failed());
    CHECK(info.heap_sector_allocated_bytes == 0);
    ns_dyn_mem_alloc(8);
    p = ns_dyn_mem_alloc(8);
    ns_dyn_mem_alloc(8);
    CHECK(info.heap_sector_allocated_bytes >= 24);
    int16_t last_value = info.heap_sector_allocated_bytes;
    ns_dyn_mem_free(p); // Free the middle one to leave hole for 4 byes.
    CHECK(info.heap_sector_allocated_bytes >= 16);
    CHECK(info.heap_sector_allocated_bytes < last_value);
    last_value = info.heap_sector_allocated_bytes;
    // Try to make allocator fill a hole bigger that requested size,
    // run alloc&free for 10 times to make it visible if the stats will not include
    // the overhead. Previously this was leading a negative value in allocated sector count.
    for (int i=0; i<10; i++) {
        p = ns_dyn_mem_alloc(1);
        ns_dyn_mem_free(p);
    }
    CHECK(info.heap_sector_allocated_bytes == last_value);
    free(heap);
}

TEST(dynmem, test_invalid_pointer_freed) {
    uint16_t size = 28;
    uint8_t *heap = (uint8_t*)malloc(size);
    CHECK(NULL != heap);
    reset_heap_error();
    ns_dyn_mem_init(heap, size, &heap_fail_callback, NULL);
    int *ptr = (int *)ns_dyn_mem_alloc(4);
    ptr--;
    *ptr = 16;
    ptr++;
    ns_dyn_mem_free(ptr);
    CHECK(NS_DYN_MEM_POINTER_NOT_VALID == current_heap_error);

    free(heap);
}

//NOTE! This test must be last!
TEST(dynmem, uninitialized_test){
    ns_dyn_mem_alloc(4);
    uint8_t buf[1];
    ns_dyn_mem_free(&buf);
}
