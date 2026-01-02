#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void die(const char* msg) {
  fprintf(stderr, "Fatal: %s\n", msg);
  exit(1);
}

void* xmalloc(size_t n) {
  void* p = malloc(n);
  if (!p) die("out of memory");
  return p;
}

int file_exists(const char* path) {
  FILE* f = fopen(path, "rb");
  if (!f) return 0;
  fclose(f);
  return 1;
}

double now_ms(void) {
#if defined(_WIN32)
  return (double)clock() * 1000.0 / (double)CLOCKS_PER_SEC;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
#endif
}
