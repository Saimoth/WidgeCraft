# WidgeCraft

WidgeCraft is a Windows-focused C++17/OpenGL 3.3 engine foundation with retained UI, SDF text, batched 2D primitives, basic 3D primitives, clipped model viewports and ray queries. The supported development target is Visual Studio 2022, Win32, Release.

## Project layout

```text
include/WidgeCraft/
  input/        Keyboard, mouse and pointer state
  window/       Window, framebuffer and client-area ownership
  ui/           Frames and retained UI widgets
  scene/        Cameras, model viewports, scene lifecycle and raycasting
  render/       Shader-program and rendering-pipeline utilities
  primitives/   Maths/types, Shapes2D, Shapes3D and SDF text

src/
  core/         Application loop and subsystem orchestration
  input/        Input implementation
  window/       Window implementation
  ui/           Frame and widget implementation
  scene/        Scene-query implementation
  render/       OpenGL shader/pipeline implementation
  primitives/   2D, 3D and text rendering implementation

sandbox/        Launchable visual integration demos
```

Compatibility headers remain at `include/WidgeCraft/*.hpp`, but new code should use the organised paths or include the aggregate `WidgeCraft/WidgeCraft.hpp` header.

## Model viewport

`ModelViewport` is the standard scene/model area for both 2D and 3D content. It owns a framebuffer rectangle, a uniformly scaled `Camera2D`, an aspect-correct perspective or orthographic `Camera3D`, world/screen conversions, ray creation and scene-text helpers.

```cpp
WidgeCraft::ModelViewport viewport;
viewport.setScreenRect(modelFrame.getAbsoluteRect());

viewport.getCamera2D().setPosition({ 0.0f, 0.0f });
viewport.getCamera2D().setZoom(1.0f);

viewport.getCamera3D().setPosition({ 6.0f, 4.0f, 9.0f });
viewport.getCamera3D().setTarget({ 0.0f, 0.0f, 0.0f });
viewport.getCamera3D().setPerspective(55.0f);

app.setRenderCallback([&](WidgeCraft::WidgeCraft& engine) {
    engine.useModelViewport(viewport);

    engine.getShapes2D().drawCircle(
        { 0.0f, 0.0f },
        100.0f,
        circleStyle);

    engine.getShapes3D().drawCube(
        { 0.0f, 0.0f, 0.0f },
        1.5f,
        cubeStyle);

    viewport.drawWorldText2D(
        engine.getTextRenderer(),
        "Map label",
        { 120.0f, 80.0f },
        24.0f);

    viewport.drawLabel3D(
        engine.getTextRenderer(),
        "Cube",
        { 0.0f, 1.0f, 0.0f },
        16.0f);
});
```

The engine uses the model rectangle as the OpenGL viewport for 3D and as the scissor rectangle for every scene pass. Large maps, terrain and projected labels are clipped at the model boundary. It then restores the full framebuffer before drawing retained UI, so UI frames, shapes and text never inherit scene transforms or stretching.

### 2D camera

`Camera2D::zoom` is pixels per world unit. Resizing the model area reveals more or less world space without changing object proportions.

```cpp
viewport.getCamera2D().setPosition({ 500.0f, 300.0f });
viewport.getCamera2D().setZoom(1.5f);

const WidgeCraft::Vec2 world =
    viewport.screenToWorld2D(engine.getInput().getMousePosition());
```

The older `SceneViewport2D` fixed-canvas helper remains available for compatibility, but `ModelViewport` is preferred for model editors, maps and visualisation tools.

### 3D camera and picking

The 3D projection always uses the current model rectangle aspect ratio.

```cpp
WidgeCraft::Ray ray = viewport.screenPointToRay3D(
    engine.getInput().getMousePosition());
```

`drawLabel3D` keeps a projected object label at a fixed pixel size. `drawWorldText3D` creates a camera-facing label whose size is derived from a requested world height.

## Primitive drawing

`Shapes2D` supports points, thick lines, triangles, rectangles and circles. `ShapeStyle2D` combines fill colour, edge colour and edge thickness.

`Shapes3D` supports lines, triangles, quads, boxes and cubes. `ShapeStyle3D` provides fill and edge styling. The 3D edge thickness uses OpenGL line width and is subject to the graphics driver's supported range.

## Scene and UI

`Scene` provides optional attach, detach, update and render hooks. Callback-based update/render code remains supported.

The retained UI contains `Frame`, `Label`, `Button` and `Checkbox`. UI rendering is isolated from model-viewport transforms and clipping.

## Generate the Visual Studio solution

Run from the repository root:

```bat
.\generate_vs2022_win32.bat
```

From Visual Studio's integrated terminal, prevent a second Visual Studio window opening with:

```bat
.\generate_vs2022_win32.bat --no-open
```

Then open or reload:

```text
build\win32-release\WidgeCraft.sln
```

The generated solution sets `widgecraft_ui_sandbox` as the startup project.

## Build

```bat
cmake --preset vs2022-win32-release
cmake --build --preset build-release --parallel
```
