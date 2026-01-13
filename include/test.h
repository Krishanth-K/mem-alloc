#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <stddef.h>

void stress_test();
void verify_pointer(void *ptr, size_t size, char pattern);
