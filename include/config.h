#pragma once

typedef struct {
  const char* model_face_detector;   // models/yunet.onnx
  const char* model_face_embedder;   // models/arcface.onnx
  const char* enroll_path;           // data/enrolled.bin

  int cam_index;                     // usually 0
  int enroll_frames;                 // number of frames to average
  int required_consecutive;          // frames needed to unlock
  float match_threshold;             // cosine distance threshold (lower is stricter)
} AppConfig;

AppConfig config_default(void);
