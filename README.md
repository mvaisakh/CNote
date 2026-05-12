# CeriumNotes 🖋️

**CeriumNotes** is a professional-grade, privacy-focused, native spatial note-taking application. It is designed for high-performance handwriting, PDF annotation, and infinite brainstorming within a secure, sandboxed environment.

---

## 🚀 Key Features

- **Infinite Spatial Canvas**: A hardware-accelerated document flow that supports continuous vertical scrolling across multi-page PDFs.
- **High-DPI Rendering Engine**: Powered by MuPDF and Qt Scene Graph, providing razor-sharp document clarity at any zoom level.
- **Vector Persistence**: Custom binary `.cerium` sidecar format for lightning-fast loading and perfect fidelity of ink strokes.
- **Secure Vault**: An app-local sandboxed directory that manages your imported documents, ensuring original files remain untouched.
- **Professional Export**: High-fidelity vector PDF flattening that merges your annotations into shareable, industry-standard PDFs.

---

## 🏗️ Architecture

CeriumNotes is built on a modern C++/Qt 6 stack with a focus on performance and spatial isolation.

### 1. Rendering Layer (`NoteCanvas`)
The heart of the app. It uses `QSGTransformNode` to manage a 3-layer scene graph:
- **PDF Layer**: Tiled rendering of document pages.
- **Static Ink Layer**: Hardware-cached vector paths for finished strokes.
- **Active Ink Layer**: Real-time feedback for the current stroke.

### 2. PDF Engine (`PdfEngine`)
A specialized MuPDF wrapper that handles:
- Multi-threaded document loading.
- High-resolution tile generation.
- Page-aware coordinate mapping.

### 3. Persistence Engine (`PersistenceManager`)
A custom binary serialization system using `QDataStream`. It automatically tracks:
- **Spatial State**: Current pan offset and zoom level.
- **Vector Data**: Comprehensive stroke metadata (color, width, points).

### 4. Document Management (`FileManager`)
Handles the secure "Vault" architecture:
- Imports PDFs via secure copying.
- Manages virtual `.note` files for blank canvas sessions.
- Organizes the Library Dashboard.

---

## 🎮 Spatial Navigation

| Action | Shortcut / Gesture |
| :--- | :--- |
| **Drawing** | Left Mouse / Stylus |
| **Scroll (Vertical)** | Mouse Wheel |
| **Zoom** | `Ctrl` + Mouse Wheel |
| **Pan (Spatial)** | Middle Mouse Drag |
| **Zoom (Touch)** | Pinch-to-Zoom |

---

## 🛠️ Build Instructions

### Dependencies
- Qt 6.5+ (Quick, Controls, Dialogs)
- MuPDF (Fitz)
- CMake 3.16+

### Build Steps
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
./CNotes
```

---

## 📜 Technical Standards

- **Privacy First**: All data is stored locally in `~/.local/share/CeriumNotes`. No telemetry, no cloud sync (unless implemented via user-controlled vault path).
- **Atomic Commits**: Strict kernel-style commit standards with signed-off-by lines and conventional prefixes.
- **Hardware Acceleration**: 100% GPU-backed rendering via the Qt Scene Graph.

---

Developed with ❤️ by the Project Cerium team.
