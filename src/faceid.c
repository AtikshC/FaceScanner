#include "faceid.h"
#include "vision_bridge.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#define MAX_DIM 1024



static float cosine_distance(const float* a, const float* b, int n) {
  double dot = 0, na = 0, nb = 0;
  for (int i = 0; i < n; i++) {
    dot += (double)a[i] * (double)b[i];
    na += (double)a[i] * (double)a[i];
    nb += (double)b[i] * (double)b[i];
  }
  double denom = sqrt(na) * sqrt(nb) + 1e-12;
  double cos_sim = dot / denom;
  return (float)(1.0 - cos_sim);
}

int faceid_enroll(const AppConfig* cfg) {
  Vision* v = vision_create(cfg->model_face_detector, cfg->model_face_embedder, cfg->cam_index);
  if (!v) die("failed to init camera/models");
  
  float tmp[MAX_DIM];
  int dim = 0;

  float* sum = NULL;
  int captured = 0;

  for (;;) {
    if (!vision_read(v)) break;

    FaceBox fb;
    int have = vision_detect_best_face(v, &fb);

    if (have) {
      int ok = vision_embed_face(v, &fb, tmp, MAX_DIM, &dim);
      printf("have=%d ok=%d dim=%d captured=%d\n", have, ok, dim, captured);

      if (ok && dim > 0) {
        if (!sum) {
          sum = (float*)xmalloc(sizeof(float) * (size_t)dim);
          for (int i = 0; i < dim; i++) sum[i] = 0.0f;
        }
        for (int i = 0; i < dim; i++) sum[i] += tmp[i];
        captured++;
      }
    }

    vision_show_enroll_overlay(v, have ? &fb : NULL, have, captured, cfg->enroll_frames);

    int k = vision_poll_key(1);
    if (k == 'q' || k == 'Q') {
      vision_destroy(v);
      if (sum) free(sum);
      return 0;
    }

    if (captured >= cfg->enroll_frames) break;
  }

  if (!sum || dim <= 0) {
    vision_destroy(v);
    if (sum) free(sum);
    die("could not enroll: no embedding captured");
  }

  // average
  for (int i = 0; i < dim; i++) sum[i] /= (float)captured;

  Embedding e;
  e.dim = dim;
  e.vec = sum;

  // ensure data/ exists is handled by user (or just create in instructions)
  if (!storage_save_embedding(cfg->enroll_path, &e)) {
    vision_destroy(v);
    free(sum);
    die("failed to save enrollment file (make sure data/ exists)");
  }

  vision_destroy(v);
  free(sum);

  printf("Enrolled successfully -> %s\n", cfg->enroll_path);
  return 1;
}

int faceid_lock(const AppConfig* cfg) {
  Embedding enrolled;
  if (!storage_load_embedding(cfg->enroll_path, &enrolled)) {
    die("no enrollment found. Run: faceid enroll (and ensure data/ exists)");
  }
  
  Vision* v = vision_create(cfg->model_face_detector, cfg->model_face_embedder, cfg->cam_index);
  if (!v) die("failed to init camera/models");

  // Create the window first by showing a frame once, then fullscreen it
  if (vision_read(v)) {
    FaceBox fb;
    int have = vision_detect_best_face(v, &fb);
    vision_show_lock_overlay(v, have ? &fb : NULL, have, 0, cfg->required_consecutive, 999.0f);
    vision_set_fullscreen(1);
  }

  
  float tmp[MAX_DIM];
  int dim = 0;
  int consecutive = 0;
  float last_dist = 999.0f;

  for (;;) {
    if (!vision_read(v)) break;

    FaceBox fb;
    int have = vision_detect_best_face(v, &fb);

    if (have) {
      int ok = vision_embed_face(v, &fb, tmp, MAX_DIM, &dim);
      if (ok && dim == enrolled.dim) {
        last_dist = cosine_distance(tmp, enrolled.vec, dim);
        if (last_dist <= cfg->match_threshold) consecutive++;
        else consecutive = 0;
      } else {
        consecutive = 0;
      }
    } else {
      consecutive = 0;
      last_dist = 999.0f;
    }

    vision_show_lock_overlay(v, have ? &fb : NULL, have, consecutive, cfg->required_consecutive, last_dist);

    int k = vision_poll_key(1);
    if (k == 'q' || k == 'Q') {
      embedding_free(&enrolled);
      vision_destroy(v);
      return 0;
    }

    if (consecutive >= cfg->required_consecutive) {
      break;
    }
  }

  embedding_free(&enrolled);
  vision_destroy(v);
  printf("Unlocked.\n");
  return 1;
}
