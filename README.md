# WidgeCraft

WidgeCraft is a Windows-focused C++17/OpenGL 3.3 engine foundation with retained UI, SDF text, batched 2D primitives, basic 3D primitives and ray queries. The supported development target is Visual Studio 2022, Win32, Release.

## Project layout

```text
include/WidgeCraft/
  input/        Keyboard, mouse and pointer state
  window/       Window, framebuffer and client-area ownership
  ui/           Frames and retained UI widgets
  scene/        Scene lifecycle, 2D viewports and world queries
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

## Primitive drawing

`Shapes2D` supports points, thick lines, triangles, rectangles and circles. The `ShapeStyle2D` helpers combine fill colour, edge colour and edge thickness in one call.

A `SceneViewport2D` maps a fixed logical canvas into the current client area with one uniform scale. This keeps all 2D scene geometry, dimensions and edge thicknesses proportional when the window aspect ratio changes:

```cpp
WidgeCraft::SceneViewport2D viewport(1100.0f, 720.0f);

viewport.resize(windowWidth, windowHeight);
auto& shapes = app.getShapes2D();
shapes.setTransform(viewport.getOffset(), viewport.getScale());

// Draw in the fixed 1100 x 720 logical scene.
shapes.drawCircle({ 200.0f, 500.0f }, 90.0f, style);
```

The retained UI automatically returns to native client-pixel coordinates after scene drawing.

`Shapes3D` supports lines, triangles, quads, boxes and cubes. Set its view-projection matrix before queuing 3D geometry:

```cpp
app.getShapes3D().setViewProjection(projection * view);
app.getShapes3D().drawCube(
    { 0.0f, 0.0f, -4.0f },
    1.5f,
    WidgeCraft::ShapeStyle3D{});
```

The 3D edge thickness uses OpenGL line width and is therefore subject to the graphics driver's supported line-width range.

## Scene and UI

`Scene` provides optional attach, detach, update and render hooks. Existing callback-based update/render code remains supported.

The retained UI contains `Frame`, `Label`, `Button` and `Checkbox`. UI drawing uses `Shapes2D` and `TextRenderer` internally.

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

The generated solution sets `widgecraft_ui_sandbox` as the startup project. The only launchable demo currently retained is:

```text
build\win32-release\Release\widgecraft_ui_sandbox.exe
```

## Build

```bat
cmake --preset vs2022-win32-release
cmake --build --preset build-release --parallel
```
