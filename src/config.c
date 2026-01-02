#include "config.h"

AppConfig config_default(void) {
  AppConfig c;
  c.model_face_detector = "models/yunet.onnx";
  c.model_face_embedder = "models/arcface.onnx";
  c.enroll_path = "data/enrolled.bin";

  c.cam_index = 0;
  c.enroll_frames = 20;
  c.required_consecutive = 8;

  // Cosine distance threshold for ArcFace embeddings
  // Typical ranges vary by model; tune after you test.
  c.match_threshold = 0.35f;
  return c;
}
