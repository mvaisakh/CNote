# CeriumNotes Development Guide

This document provides a technical overview of the CeriumNotes codebase for developers.

## 📐 Spatial Coordinate System

CeriumNotes uses a "Continuous Canvas" coordinate system. 

- **Canvas Space**: An infinite coordinate system where (0,0) is the top-left of the first page.
- **Screen Space**: The pixel-coordinates of the application window.
- **Mapping**:
  - `mapToCanvas()`: Converts screen mouse/stylus positions into canvas coordinates by applying inverse `m_panOffset` and `m_zoomLevel`.
  - `mapFromCanvas()`: Converts canvas points back to screen pixels for rendering.

### Vertical Flow
Pages are stacked vertically with a `spacing` constant.
`PageY = Sum(PageH[0...i-1]) + (i * spacing)`

## 💾 Persistence Format (.cerium)

The `.cerium` sidecar file is a binary stream versioned at `V1`.

**Header**: `0xCE71011` (Magic Number)
**Structure**:
1. `float` Zoom Level
2. `QPointF` Pan Offset
3. `int` Stroke Count
4. **Stroke Data** (Loop):
   - `QColor` Color
   - `float` Width
   - `int` Point Count
   - **Point Data** (Loop):
     - `float` X
     - `float` Y
     - `float` Pressure
     - `float` Tilt
     - `qint64` Timestamp

## 🎨 Scene Graph Architecture

The `NoteCanvas::updatePaintNode` method is called by the Qt Quick thread to synchronize the visual state.

1. **QSGTransformNode (Root)**: Applies the global `m_panOffset` and `m_zoomLevel`.
2. **Layer 0 (PDF)**: Contains `QSGSimpleTextureNode` instances for each visible page.
3. **Layer 1 (Static Ink)**: Contains thousands of `QSGGeometryNode` instances. Optimized using a `m_fullReload` flag to prevent rebuilding the entire scene graph on every stroke.
4. **Layer 2 (Active Ink)**: A single `QSGGeometryNode` that is updated every frame during a drawing gesture.

## 📄 Export Engine Logic

The `ExportEngine` uses MuPDF's low-level `pdf_page_write` API to perform "Flattening":
1. Original page content is recorded into a `fz_display_list`.
2. Ink strokes are converted into `fz_path` objects.
3. Both are re-played onto a new PDF document using a `pdf_device`.
4. Coordinates are re-mapped by subtracting the `currentYOffset` of the page from the continuous canvas ink points.

---

## 🛠️ Contribution Rules
- Always run a build before committing.
- Use `CN_TRACE` for spatial debugging.
- Maintain the `Main.cpp` clean by relying on the Qt QML Module system.
