# FaceID Desktop (C + AI) — Windows

A FaceID-style desktop lock/unlock demo for Windows:
- Detects your face in real-time (YuNet / OpenCV)
- Extracts a face embedding (ArcFace / ONNX Runtime)
- **Enroll** creates a local profile file
- **Lock** shows a fullscreen lock overlay and unlocks when your face matches

> Note: This does **not** replace Windows login / Windows Hello. It’s an app-level lock overlay demo.

---

## Features
- Real-time webcam preview + face bounding box
- Enrollment: averages multiple embeddings for stability
- Verification: cosine distance + consecutive-frame requirement
- Fullscreen “LOCKED” overlay
- Works offline (local ONNX models)

---

## Project Structure (important folders)
- `src/` — C app logic (`faceid.c`, config, storage, etc.)
- `bridge/` — OpenCV + ONNX Runtime bridge (`vision_bridge.cpp`)
- `include/` — headers
- `models/` — **YOU download the ONNX models here (not committed)**
- `data/` — enrollment output `enrolled.bin` (not committed)
- `build/` — build outputs (not committed)

---
## Models

Download the required ONNX models and place them in `models/`:

- arcface.onnx
- yunet.onnx

These files are not tracked due to size limits.


## Requirements (Windows)
- Windows 10/11 (x64)
- Visual Studio 2022 (C++ workload)
- CMake
- Git
- vcpkg (for OpenCV)
- ONNX Runtime (C/C++ package)

---

## 1) Install Tools

### Visual Studio 2022
Install **Visual Studio 2022** with:
- ✅ Desktop development with C++

### CMake
Install Kitware CMake (or VS “CMake tools”), then verify:
```powershell
cmake --version
Git
Install Git for Windows, then verify:

powershell
Copy code
git --version
2) Install OpenCV using vcpkg
This repo uses vcpkg to install OpenCV.

From the vcpkg folder (example path shown):

powershell
Copy code
cd C:\Projects\Face_scanner\vcpkg
.\vcpkg.exe install opencv4:x64-windows
Optional (extra video codecs/features):

powershell
Copy code
.\vcpkg.exe install opencv4[contrib,ffmpeg]:x64-windows
3) Download AI Models (put in models/)
A) Face Detector: YuNet (ONNX)
Download the YuNet ONNX model and place it as:

models\yunet.onnx

(If the downloaded file is named like face_detection_yunet_2023mar.onnx, rename it to yunet.onnx.)

B) Face Embedder: ArcFace (ONNX)
Download an ArcFace ONNX embedding model and place it as:

models\arcface.onnx

Important: Your ArcFace model must accept input shape [1,112,112,3] (NHWC) based on this project’s current preprocessing.

4) Download ONNX Runtime (C/C++ package)
Download the Windows x64 ONNX Runtime zip that includes headers:

onnxruntime-win-x64-<version>.zip

Extract it somewhere, for example:

C:\Users\atiks\Downloads\onnxruntime-win-x64-1.23.2\onnxruntime-win-x64-1.23.2\

Verify it contains:

include\onnxruntime_cxx_api.h

lib\onnxruntime.lib

lib\onnxruntime.dll

5) Build (Windows / Visual Studio Generator)
Configure
From the repo root:

powershell
Copy code
cd C:\Projects\Face_scanner\face-id-desktop
mkdir build -ErrorAction SilentlyContinue
cd build
Configure with your vcpkg toolchain + ONNX Runtime dir:

powershell
Copy code
& "C:\Program Files\CMake\bin\cmake.exe" .. -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="C:/Projects/Face_scanner/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DONNXRUNTIME_DIR="C:/Users/atiks/Downloads/onnxruntime-win-x64-1.23.2/onnxruntime-win-x64-1.23.2"
Build Release
powershell
Copy code
& "C:\Program Files\CMake\bin\cmake.exe" --build . --config Release --target faceid
Your exe should be here:

build\Release\faceid.exe

6) Runtime DLL Setup (Windows)
If running the program gives a missing DLL error (common on Windows), copy ONNX Runtime DLL next to the exe:

powershell
Copy code
copy "C:\Users\atiks\Downloads\onnxruntime-win-x64-1.23.2\onnxruntime-win-x64-1.23.2\lib\onnxruntime.dll" `
     "C:\Projects\Face_scanner\face-id-desktop\build\Release\"
7) Run
Create data/ folder
From the repo root:

powershell
Copy code
cd C:\Projects\Face_scanner\face-id-desktop
mkdir data -ErrorAction SilentlyContinue
A) Enroll (create your profile)
powershell
Copy code
.\build\Release\faceid.exe enroll
This should create:

data\enrolled.bin

Tips:

Use good lighting

Look at the camera, hold still

Press Q to quit

B) Lock / Unlock
powershell
Copy code
.\build\Release\faceid.exe lock
A fullscreen “LOCKED” overlay appears

It unlocks when your face matches for enough consecutive frames

Press Q to quit

Configuration / Tuning
Edit src/config.c and rebuild.

Common settings:

cam_index (0, 1, 2...) if your webcam isn’t the default

match_threshold (higher = easier unlock, lower = stricter)

Try 0.45 if it won’t unlock

Try 0.30–0.35 if it unlocks too easily

required_consecutive (more = more stable, slower unlock)

Rebuild after changes:

powershell
Copy code
cd .\build
& "C:\Program Files\CMake\bin\cmake.exe" --build . --config Release --target faceid
Troubleshooting
“Could not find OpenCVConfig.cmake”
Install OpenCV via the same vcpkg instance + x64 triplet:

powershell
Copy code
cd C:\Projects\Face_scanner\vcpkg
.\vcpkg.exe install opencv4:x64-windows
“onnxruntime_cxx_api.h not found”
Your ONNXRUNTIME_DIR must point to the folder that contains include/.
Confirm:

powershell
Copy code
dir "<ONNXRUNTIME_DIR>\include\onnxruntime_cxx_api.h"
“onnxruntime.dll missing”
Copy the DLL next to the exe (see Runtime DLL Setup).

Face box shows but enroll stays at 0/20
Embedding inference failed. This project expects ArcFace input NHWC [1,112,112,3].
Try a different ArcFace ONNX model if needed.

Security Notes
This is a demo and is not a secure OS authentication boundary.

The enrollment file data/enrolled.bin is a biometric template (embedding). Keep it private.

License
MIT


