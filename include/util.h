#pragma once
#include <stddef.h>

void die(const char* msg);
void* xmalloc(size_t n);
int file_exists(const char* path);

double now_ms(void);
