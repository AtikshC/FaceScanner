#pragma once
#include <stddef.h>

typedef struct {
  int dim;
  float* vec; // length dim
} Embedding;

void embedding_free(Embedding* e);

int storage_save_embedding(const char* path, const Embedding* e);
int storage_load_embedding(const char* path, Embedding* out);


