#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <stddef.h>

void verify_pointer(void *ptr, size_t size, char pattern);
void test_alignment(void);
void test_calloc(void);
void test_realloc(void);
void test_edge_cases(void);
void test_coalescing(void);
void test_boundaries(void);
void stress_test(void);
void fragmentation_test(void);
void comprehensive_test(void);
