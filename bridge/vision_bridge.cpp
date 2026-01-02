#include "vision_bridge.h"

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

#include <onnxruntime_cxx_api.h>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#ifdef _WIN32
  #include <windows.h>
  #include <string>
#endif
#include <opencv2/objdetect.hpp>
#include <array>
#include <cstdio>


struct Vision {
  cv::VideoCapture cap;
  cv::Mat frame_bgr;

  // Face detector (YuNet ONNX via OpenCV DNN)
  cv::Ptr<cv::FaceDetectorYN> detector;
  int det_w = 320;
  int det_h = 320;


  // Embedder (ArcFace ONNX via ONNX Runtime)
  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "faceid"};
  Ort::SessionOptions sess_opts;
  Ort::Session* session = nullptr;

  std::string input_name;
  std::string output_name;

  int embed_w = 112;
  int embed_h = 112;

  Vision() {
    sess_opts.SetIntraOpNumThreads(1);
    sess_opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
  }
};

static cv::Mat center_crop_resize(const cv::Mat& img, int w, int h) {
  cv::Mat resized;
  cv::resize(img, resized, cv::Size(w, h));
  return resized;
}

extern "C" Vision* vision_create(const char* face_detector_onnx, const char* face_embedder_onnx, int cam_index) {
  auto* v = new Vision();

  v->cap.open(cam_index);
  if (!v->cap.isOpened()) {
    delete v;
    return nullptr;
  }

  try {
    v->detector = cv::FaceDetectorYN::create(
  face_detector_onnx,
  "",                       // no config
  cv::Size(v->det_w, v->det_h),
  0.3f,                     // score threshold 
  0.3f,                     // nms threshold
  5000                      // top_k
);
if (v->detector.empty()) {
  delete v;
  return nullptr;
}

  } catch (...) {
    delete v;
    return nullptr;
  }

  try {
    #ifdef _WIN32
  // Convert UTF-8 char* path to wide char* for ORT on Windows
  int wlen = MultiByteToWideChar(CP_UTF8, 0, face_embedder_onnx, -1, NULL, 0);
  std::wstring wpath(wlen, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, face_embedder_onnx, -1, &wpath[0], wlen);

  v->session = new Ort::Session(v->env, wpath.c_str(), v->sess_opts);
#else
  v->session = new Ort::Session(v->env, face_embedder_onnx, v->sess_opts);
#endif


    Ort::AllocatorWithDefaultOptions allocator;

auto in0 = v->session->GetInputNameAllocated(0, allocator);
auto out0 = v->session->GetOutputNameAllocated(0, allocator);

v->input_name = in0 ? in0.get() : "";
v->output_name = out0 ? out0.get() : "";

  } catch (...) {
    delete v;
    return nullptr;
  }

  return v;
}

extern "C" void vision_destroy(Vision* v) {
  if (!v) return;
  if (v->session) delete v->session;
  delete v;
}

extern "C" int vision_read(Vision* v) {
  if (!v) return 0;
  v->cap >> v->frame_bgr;
  return !v->frame_bgr.empty();
}

extern "C" int vision_detect_best_face(Vision* v, FaceBox* out) {
  if (!v || v->frame_bgr.empty()) return 0;

  // YuNet detector needs to know the current frame size
  v->detector->setInputSize(v->frame_bgr.size());

  cv::Mat faces; // Nx15 float matrix
  v->detector->detect(v->frame_bgr, faces);

  if (faces.empty() || faces.rows <= 0) return 0;

  float best = 0.0f;
  FaceBox bestBox{0,0,0,0,0};

  for (int i = 0; i < faces.rows; i++) {
    float x = faces.at<float>(i, 0);
    float y = faces.at<float>(i, 1);
    float w = faces.at<float>(i, 2);
    float h = faces.at<float>(i, 3);
    float s = faces.at<float>(i, 4);

    if (s > best) {
      best = s;
      bestBox.x = (int)x;
      bestBox.y = (int)y;
      bestBox.w = (int)w;
      bestBox.h = (int)h;
      bestBox.score = s;
    }
  }

  if (best < 0.5f || bestBox.w <= 0 || bestBox.h <= 0) return 0;
  *out = bestBox;
  return 1;
}


static float cosine_distance(const float* a, const float* b, int n) {
  double dot = 0, na = 0, nb = 0;
  for (int i = 0; i < n; i++) {
    dot += (double)a[i] * (double)b[i];
    na += (double)a[i] * (double)a[i];
    nb += (double)b[i] * (double)b[i];
  }
  double denom = std::sqrt(na) * std::sqrt(nb) + 1e-12;
  double cos_sim = dot / denom;
  return (float)(1.0 - cos_sim); // distance
}

extern "C" int vision_embed_face(Vision* v, const FaceBox* face, float* out_vec, int out_cap, int* out_dim) {
  if (!v || v->frame_bgr.empty() || !face || !out_vec || !out_dim) return 0;

  cv::Rect r(face->x, face->y, face->w, face->h);
  r &= cv::Rect(0, 0, v->frame_bgr.cols, v->frame_bgr.rows);
  if (r.width <= 0 || r.height <= 0) return 0;

  // Crop -> 112x112 -> RGB float32 -> normalize to [-1, 1]
  cv::Mat crop = v->frame_bgr(r).clone();
  cv::resize(crop, crop, cv::Size(v->embed_w, v->embed_h));
  cv::cvtColor(crop, crop, cv::COLOR_BGR2RGB);
  crop.convertTo(crop, CV_32F);

  // ArcFace standard normalization: (x - 127.5) / 127.5
  crop = (crop - 127.5f) / 127.5f;

  // Pack NCHW: [1,3,112,112]
 // Pack NHWC: [1,112,112,3]
const int H = v->embed_h;
const int W = v->embed_w;
std::vector<float> input(1 * H * W * 3);
size_t idx = 0;
for (int y = 0; y < H; y++) {
  for (int x = 0; x < W; x++) {
    const cv::Vec3f pix = crop.at<cv::Vec3f>(y, x);
    input[idx++] = pix[0]; // R
    input[idx++] = pix[1]; // G
    input[idx++] = pix[2]; // B
  }
}

std::array<int64_t, 4> shape{1, (int64_t)H, (int64_t)W, 3};
Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(
    OrtArenaAllocator, OrtMemTypeDefault
);

  Ort::Value in_tensor = Ort::Value::CreateTensor<float>(
      mem, input.data(), input.size(), shape.data(), shape.size());

  try {
    // Always query the real names from the model (no guessing)
    Ort::AllocatorWithDefaultOptions allocator;
    auto in0 = v->session->GetInputNameAllocated(0, allocator);
    auto out0 = v->session->GetOutputNameAllocated(0, allocator);

    const char* in_names[] = { in0.get() };
    const char* out_names[] = { out0.get() };

    auto outs = v->session->Run(
        Ort::RunOptions{nullptr},
        in_names, &in_tensor, 1,
        out_names, 1
    );

    if (outs.empty()) {
      fprintf(stderr, "ORT Run: no outputs\n");
      return 0;
    }

    auto info = outs[0].GetTensorTypeAndShapeInfo();
    size_t total = info.GetElementCount();

    if (total == 0 || total > (size_t)out_cap) {
      fprintf(stderr, "ORT output invalid: total=%zu cap=%d\n", total, out_cap);
      return 0;
    }

    float* data = outs[0].GetTensorMutableData<float>();
    for (size_t i = 0; i < total; i++) out_vec[i] = data[i];

    *out_dim = (int)total;
    return 1;
  } catch (const Ort::Exception& e) {
    fprintf(stderr, "ORT exception: %s\n", e.what());
    return 0;
  } catch (...) {
    fprintf(stderr, "Unknown exception in vision_embed_face\n");
    return 0;
  }
}


extern "C" void vision_show_enroll_overlay(Vision* v, const FaceBox* face, int have_face, int captured, int target) {
  if (!v || v->frame_bgr.empty()) return;

  cv::Mat show = v->frame_bgr.clone();
  if (have_face && face) {
    cv::rectangle(show, cv::Rect(face->x, face->y, face->w, face->h), cv::Scalar(0,255,0), 2);
  }

  std::string msg = "Enroll: look at camera (" + std::to_string(captured) + "/" + std::to_string(target) + ")";
  cv::putText(show, msg, cv::Point(20, 40), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255,255,255), 2);
  cv::imshow("FaceID", show);
}

extern "C" void vision_show_lock_overlay(Vision* v, const FaceBox* face, int have_face, int consecutive, int required, float last_dist) {
  if (!v || v->frame_bgr.empty()) return;

  cv::Mat show = v->frame_bgr.clone();
  // darken
  show = show * 0.25;

  if (have_face && face) {
    cv::rectangle(show, cv::Rect(face->x, face->y, face->w, face->h), cv::Scalar(0,255,0), 2);
  }

  cv::putText(show, "LOCKED", cv::Point(20, 60), cv::FONT_HERSHEY_SIMPLEX, 2.0, cv::Scalar(255,255,255), 3);
  std::string msg = "Match streak: " + std::to_string(consecutive) + "/" + std::to_string(required) +
                    "  dist=" + std::to_string(last_dist);
  cv::putText(show, msg, cv::Point(20, 110), cv::FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(255,255,255), 2);
  cv::putText(show, "Press Q to quit", cv::Point(20, 150), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255,255,255), 2);

  cv::imshow("FaceID", show);
}

extern "C" void vision_set_fullscreen(int fullscreen) {
  // best-effort: set window fullscreen after it's created
  int flag = fullscreen ? cv::WINDOW_FULLSCREEN : cv::WINDOW_NORMAL;
  cv::setWindowProperty("FaceID", cv::WND_PROP_FULLSCREEN, flag);
}

extern "C" int vision_poll_key(int delay_ms) {
  int k = cv::waitKey(delay_ms);
  if (k < 0) return -1;
  return k & 0xFF;
}
