cmake_minimum_required(VERSION 3.16)
project(faceid_desktop C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# ---- OpenCV ----
find_package(OpenCV REQUIRED)

# ---- ONNX Runtime ----
# You must set ONNXRUNTIME_DIR to your ONNX Runtime install path
# Example:
#   -DONNXRUNTIME_DIR="C:/onnxruntime-win-x64-1.17.3"
if (NOT DEFINED ONNXRUNTIME_DIR)
  message(FATAL_ERROR "ONNXRUNTIME_DIR not set. Pass -DONNXRUNTIME_DIR=...</onnxruntime>")
endif()

set(ONNXRUNTIME_INCLUDE "${ONNXRUNTIME_DIR}/include")
set(ONNXRUNTIME_LIB_DIR "${ONNXRUNTIME_DIR}/lib")

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${ONNXRUNTIME_INCLUDE})
include_directories(${ONNXRUNTIME_INCLUDE}/onnxruntime)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(faceid
  src/main.c
  src/app.c
  src/config.c
  src/faceid.c
  src/storage.c
  src/util.c
  bridge/vision_bridge.cpp
)

# Windows: onnxruntime.lib ; Linux/macOS: libonnxruntime.so/.dylib
if (WIN32)
  target_link_libraries(faceid PRIVATE ${OpenCV_LIBS} "${ONNXRUNTIME_LIB_DIR}/onnxruntime.lib")
else()
  target_link_libraries(faceid PRIVATE ${OpenCV_LIBS} "${ONNXRUNTIME_LIB_DIR}/libonnxruntime.so")
endif()
