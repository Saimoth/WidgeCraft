# WidgeCraft

WidgeCraft is a small retained-mode graphics and UI engine for Windows, C++17 and OpenGL 3.3 Core. The project is deliberately focused: useful pixel-space primitives, scalable SDF text, a built-in widget tree, and clean ray queries without depending on an immediate-mode UI library.

The current supported development target is **Visual Studio 2022, Win32, Release**.

## Current engine foundation

### Batched 2D primitives

`ShapeRenderer` batches coloured triangles and builds the common primitives on top:

- Points
- Thick lines
- Filled triangles
- Filled and outlined rectangles
- Filled and outlined circles with adaptive tessellation

```cpp
auto& shapes = app.getShapeRenderer();
shapes.drawLine(20.0f, 20.0f, 220.0f, 80.0f, 4.0f, WidgeCraft::Colors::Cyan);
shapes.drawFilledCircle(320.0f, 180.0f, 48.0f, WidgeCraft::Colors::Orange);
shapes.drawRectOutline(400.0f, 100.0f, 160.0f, 90.0f, 3.0f, WidgeCraft::Colors::Green);
```

### SDF text

`TextRenderer` creates a signed-distance-field atlas from a TrueType font and batches glyphs into one draw per layer. It includes:

- Smooth scaling from small labels to large headings
- Mipmapped atlas sampling
- Derivative-based edge smoothing
- Kerning
- UTF-8 decoding for the bundled Western/common-symbol atlas
- Text bounds and width measurement
- Per-glyph colour and alpha

```cpp
auto& text = app.getTextRenderer();
text.renderText("Small and crisp", 30.0f, 80.0f, 12.0f, WidgeCraft::Colors::White);
text.renderText("Large SDF", 30.0f, 180.0f, 72.0f, { 0.3f, 0.8f, 1.0f, 1.0f });
```

### Retained UI

Frames own child frames and widgets. Frames and widgets support anchors and offsets in a bottom-left pixel coordinate system. Frame contents are clipped using OpenGL scissor rectangles.

Included widgets:

- `Label`
- `Button` with hover, press, release and click callback states
- `Checkbox` with a change callback

```cpp
auto& panel = app.getRootFrame().createChildFrame("Inspector");
panel.setAnchor(WidgeCraft::Anchor::TopRight);
panel.setPosition(20.0f, 20.0f);
panel.setSize(300.0f, 220.0f);

auto& button = panel.addWidget<WidgeCraft::Button>("Apply", "Apply");
button.setAnchor(WidgeCraft::Anchor::BottomRight);
button.setPosition(16.0f, 16.0f);
button.setSize(100.0f, 38.0f);
button.setOnClick([] {
    // Apply changes.
});
```

Names are unique within each parent frame. Frames and widgets can be found or removed by name.

### Input

`Input` records frame transitions rather than exposing only raw GLFW state:

```cpp
const auto& input = app.getInput();
if (input.mousePressed(WidgeCraft::MouseButton::Left)) {
    const WidgeCraft::Vec2 position = input.mousePosition();
}
```

Available state includes key and mouse `down`, `pressed` and `released`, pointer position/delta, and wheel delta.

### Raycasting and ray picking

The ray module is independent from OpenGL and can be unit tested without opening a window. It currently includes:

- Ray versus sphere
- Ray versus axis-aligned bounding box
- Ray versus plane
- Ray versus triangle, with optional back-face culling
- Screen point to world ray using view and projection matrices
- Matrix inversion and hit point/normal/distance data

```cpp
WidgeCraft::Ray ray{ cameraPosition, rayDirection };
WidgeCraft::RayHit hit{};

if (WidgeCraft::raycast(ray, objectBounds, hit)) {
    // hit.distance, hit.point and hit.normal are available here.
}
```

## Generate the Visual Studio solution

From a Visual Studio Developer Command Prompt:

```bat
cmake --preset vs2022-win32-release
```

Or double-click:

```text
generate_vs2022_win32.bat
```

Open:

```text
build\win32-release\WidgeCraft.sln
```

The main targets are:

- `widgecraft_engine` / `WidgeCraft::Engine` — reusable static engine library
- `widgecraft` — interactive showcase
- `widgecraft_raycast_tests` — non-graphical ray tests

## Build and test

```bat
cmake --build --preset build-release --parallel
ctest --preset test-release
```

The showcase executable is generated at:

```text
build\win32-release\Release\widgecraft.exe
```

Assets are copied beside the executable after a successful build.

## Coordinate conventions

- 2D screen and UI coordinates use a **bottom-left origin**.
- Positive X points right; positive Y points up.
- Text X/Y is its baseline origin.
- GLFW cursor coordinates are converted into framebuffer pixels, including high-DPI scaling.
- Matrices use OpenGL-compatible column-major storage.

## Project structure

```text
assets/                 Fonts and future engine assets
include/WidgeCraft/     Public engine headers
src/                    Engine implementation and showcase
third_party/            Header-only third-party source
tests/                  Non-graphical unit tests
.github/workflows/      Win32 CI build and tests
```

## Near-term direction

This branch establishes the engine foundation rather than trying to hide unfinished areas. Logical next additions are a dedicated 3D model renderer and camera, texture/image primitives, keyboard focus and navigation, richer text ranges, movable/closable windows, and a broad-phase structure such as a BVH for large ray-picking scenes.
