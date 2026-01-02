
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Vision Vision;

typedef struct {
  int x, y, w, h;
  float score;
} FaceBox;

// Create/destroy
Vision* vision_create(const char* face_detector_onnx, const char* face_embedder_onnx, int cam_index);
void vision_destroy(Vision* v);

// Grab next frame internally, return 1 if ok
int vision_read(Vision* v);

// Detect the best face. returns 1 if found.
int vision_detect_best_face(Vision* v, FaceBox* out);

// Compute embedding for a detected face. returns 1 if ok.
// out_vec must have capacity at least out_cap floats.
// Writes actual dim to *out_dim.
int vision_embed_face(Vision* v, const FaceBox* face, float* out_vec, int out_cap, int* out_dim);

// UI
void vision_show_enroll_overlay(Vision* v, const FaceBox* face, int have_face, int captured, int target);
void vision_show_lock_overlay(Vision* v, const FaceBox* face, int have_face, int consecutive, int required, float last_dist);

// Window control
void vision_set_fullscreen(int fullscreen);

// Key handling
// returns ASCII code or -1 if none (uses OpenCV waitKey)
int vision_poll_key(int delay_ms);

#ifdef __cplusplus
}
#endif
