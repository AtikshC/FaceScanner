#include "storage.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

void embedding_free(Embedding* e) {
  if (!e) return;
  if (e->vec) {
    free(e->vec);
    e->vec = NULL;
  }
  e->dim = 0;
}

int storage_save_embedding(const char* path, const Embedding* e) {
  FILE* f = fopen(path, "wb");
  if (!f) return 0;

  const unsigned int magic = 0xFAD1D00D;
  fwrite(&magic, sizeof(magic), 1, f);
  fwrite(&e->dim, sizeof(e->dim), 1, f);
  fwrite(e->vec, sizeof(float), (size_t)e->dim, f);

  fclose(f);
  return 1;
}

int storage_load_embedding(const char* path, Embedding* out) {
  memset(out, 0, sizeof(*out));
  FILE* f = fopen(path, "rb");
  if (!f) return 0;

  unsigned int magic = 0;
  if (fread(&magic, sizeof(magic), 1, f) != 1) { fclose(f); return 0; }
  if (magic != 0xFAD1D00D) { fclose(f); return 0; }

  int dim = 0;
  if (fread(&dim, sizeof(dim), 1, f) != 1) { fclose(f); return 0; }
  if (dim <= 0 || dim > 4096) { fclose(f); return 0; }

  float* vec = (float*)xmalloc(sizeof(float) * (size_t)dim);
  if (fread(vec, sizeof(float), (size_t)dim, f) != (size_t)dim) {
    fclose(f);
    free(vec);
    return 0;
  }

  fclose(f);
  out->dim = dim;
  out->vec = vec;
  return 1;
}
