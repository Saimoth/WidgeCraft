# WidgeCraft

WidgeCraft is a Windows-focused C++17/OpenGL 3.3 engine foundation with managed scenes, objects and UI screens, heightmap terrain, lightweight AABB/heightfield physics, OBJ/STL models, retained UI, SDF text, batched 2D and 3D primitives, independently clipped model viewports and ray queries. The supported development target is Visual Studio 2022, Win32, Release.

## Project layout

```text
include/WidgeCraft/
  input/        Keyboard, mouse and pointer state
  model/        Shared meshes, transforms and OBJ/STL loading
  physics/      Box/heightfield collision, triggers, bodies and gravity
  terrain/      PGM heightmaps, gradual generation and terrain meshes
  window/       Window, framebuffer and client-area ownership
  ui/           UI screens, scene views, frames and retained widgets
  scene/        Scene/object management, cameras, viewports and raycasting
  render/       Shader-program and rendering-pipeline utilities
  primitives/   Maths/types, Shapes2D, Shapes3D and SDF text

src/
  core/         Application loop and subsystem orchestration
  input/        Input implementation
  model/        Mesh transforms and model parsers
  physics/      Sub-stepped physics simulation and collision solver
  terrain/      Height sampling, generation and mesh construction
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
    engine.renderViewport(viewport, [&](WidgeCraft::WidgeCraft& pass) {
        pass.getShapes2D().drawCircle(
            { 0.0f, 0.0f },
            100.0f,
            circleStyle);

        pass.getShapes3D().drawCube(
            { 0.0f, 0.0f, 0.0f },
            1.5f,
            cubeStyle);

        viewport.drawWorldText2D(
            pass.getTextRenderer(),
            "Map label",
            { 120.0f, 80.0f },
            24.0f);

        viewport.drawLabel3D(
            pass.getTextRenderer(),
            "Cube",
            { 0.0f, 1.0f, 0.0f },
            16.0f);
    });
});
```

Each `renderViewport` call is an independent pass. The engine uses that rectangle as the OpenGL viewport for 3D and as the scissor rectangle for 2D, 3D and text, then flushes the pass before another camera can be selected. Large maps, terrain and projected labels are clipped at the viewport boundary. Geometry and glyphs remain batched within each pass.

The legacy `useModelViewport` API remains supported. Selecting a second viewport now flushes the first one automatically, so different cameras can no longer accidentally share the last viewport state.

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

### Draw distance

`ModelViewport::setDrawDistance` updates the 3D camera far plane. `ObjectManager::setDrawDistance` performs the matching CPU distance test, so out-of-range objects are not transformed, queued or labelled before the GPU clips the view.

```cpp
viewport.setDrawDistance(80.0f);
objects.setDrawDistance(80.0f);

const auto stats = objects.render(
    engine.getShapes3D(),
    viewport.getCamera3D().getPosition(),
    &viewport,
    &engine.getTextRenderer());
```

`ObjectRenderStats` reports considered, rendered and culled objects, queued triangles and labels, temporary vertex bytes, and CPU queue time.

## Primitive drawing

`Shapes2D` supports points, thick lines, triangles, rectangles and circles. `ShapeStyle2D` combines fill colour, edge colour and edge thickness.

`Shapes3D` supports lines, triangles, quads, boxes, cubes and shared `Mesh3D` objects. All filled triangles queued in one viewport pass share one GPU upload/draw. `Shapes2D::drawMesh` does the same for `Mesh2D`. `ShapeStyle3D` provides fill and edge styling. The 3D edge thickness uses OpenGL line width and is subject to the graphics driver's supported range.

## Models and managed objects

`ModelLoader` reads Wavefront OBJ faces (including polygon fan triangulation and negative indices), ASCII STL and binary STL. A loaded 3D mesh can also be projected onto the XY, XZ or YZ plane for batched 2D drawing.

```cpp
auto mesh = std::make_shared<WidgeCraft::Mesh3D>(
    WidgeCraft::ModelLoader::load("assets/models/pyramid.obj"));

WidgeCraft::ObjectManager objects(&physics);
auto& object = objects.createMeshObject("Pyramid", mesh);
object.setPosition({ 4.0f, 1.0f, -3.0f });
object.getAttributes().collidable = true;
object.getAttributes().fixed = true;
object.getAttributes().labelVisible = true;
```

Every managed object has `enabled`, `rendered`, `interactable`, `collidable`, `fixed`, `gravity`, `trigger`, `selectable` and `labelVisible` attributes. `preparePhysics()` creates or updates the required bodies before `PhysicsWorld::step`; `syncFromPhysics()` copies dynamic body positions back afterwards. Trigger bodies report overlap in `PhysicsCollision::trigger` without applying collision response.

`raycastInteractable` only considers enabled, interactable/selectable objects. `queryTriggers` provides a physics-independent AABB trigger query, while `getTriggeredObjects` maps current physics trigger overlaps back to object IDs.

## Heightmap terrain

`HeightMap` can load 8-bit P2/P5 PGM files or create reproducible gradual random terrain. The `maximumNeighbourStep` generator argument limits adjacent vertex height changes before optional smoothing.

```cpp
auto heights = std::make_shared<WidgeCraft::HeightMap>(
    WidgeCraft::HeightMap::generateGradual(
        65, 65, 1.0f, 0.58f, 1337, 3,
        { -32.0f, 0.0f, -32.0f }));

WidgeCraft::Terrain terrain(heights);
physics.setHeightMapCollider(heights);
```

The rectangular grid is split into alternating right triangles. A heightmap naturally supplies square rows and columns; alternating the diagonal avoids a consistent directional bias while preserving compact indexing and exact collision sampling. An equilateral layout would require staggered samples and would not map cleanly to ordinary heightmap images.

Dynamic AABB bodies sample the same triangle surface used to render `Terrain`, so gravity, grounded state and jumping work on gradual slopes. `Terrain::draw` also accepts a CPU draw distance for large grids.

## Physics

`PhysicsWorld` owns static and dynamic bodies with local `BoxCollider` bounds. Dynamic bodies integrate gravity, expose velocity and grounded state, and resolve AABB overlap by the minimum translation axis. The world automatically splits long frames into small steps to reduce tunnelling and lets moving bodies slide along obstacles.

```cpp
WidgeCraft::PhysicsWorld physics;

physics.createBody(
    WidgeCraft::PhysicsBodyType::Static,
    { 0.0f, -0.5f, 0.0f },
    WidgeCraft::BoxCollider({ 30.0f, 0.5f, 30.0f }));

auto& player = physics.createBody(
    WidgeCraft::PhysicsBodyType::Dynamic,
    { 0.0f, 0.9f, 5.0f },
    WidgeCraft::BoxCollider({ 0.55f, 0.9f, 0.55f }),
    1);

physics.step(engine.getDeltaTime());
```

`PhysicsBody::getBounds()` returns the same world-space `AABB` accepted by `raycast`, so picking and collision can share one authoritative bound. Box bodies remain axis-aligned and do not have rotational dynamics; terrain collision is sampled from the heightmap rather than using a general triangle-mesh solver.

## Scene and UI

`SceneManager` stores named `Scene` objects and keeps their state alive while another scene is active. `UiManager` does the same for named `UiScreen` objects. Both provide attach, detach, update and render lifecycles.

```cpp
app.getSceneManager().emplace<LoadingScene>("loading", gameState);
app.getSceneManager().emplace<WorldScene>("world", gameState);
app.getUiManager().emplace<CharacterUi>("character", app, gameState);
app.getUiManager().emplace<CombatUi>("combat", app, gameState);

app.getSceneManager().activate("loading");
app.getUiManager().activate("character");
```

Use requests from scene or widget callbacks. They are applied together at the next safe frame boundary, after the current callback has returned.

```cpp
loadButton.setOnClick([&app]() {
    app.getSceneManager().request("world");
    app.getUiManager().request("combat");
});
```

### Scene views inside UI

`SceneView` binds a `ModelViewport` to a retained `Frame`. It follows the frame's anchor, size, visibility and clipping hierarchy, renders before the frame, and lets the frame's border and widgets overlay its contents. This is intended for minimaps, model previews, inventory characters and editor panels.

```cpp
auto& mapFrame = getRootFrame().createChildFrame("Minimap Frame");
mapFrame.setAnchor(WidgeCraft::Anchor::TopRight);
mapFrame.setSize(280.0f, 220.0f);

auto& minimap = createSceneView("Minimap", mapFrame);
minimap.setInset(4.0f);
minimap.getViewport().getCamera2D().setZoom(8.0f);
minimap.setRenderCallback([](
    WidgeCraft::WidgeCraft& engine,
    const WidgeCraft::SceneView& view) {
    engine.getShapes2D().drawCircle(
        playerPosition,
        1.0f,
        playerStyle);
    view.getViewport().drawLabel2D(
        engine.getTextRenderer(),
        "Player",
        playerPosition,
        14.0f);
});
```

Inactive UI screens do not render or process widget input. The retained UI contains `Frame`, `Label`, `Button`, `Checkbox` and `Slider`, and remains isolated from scene transforms and clipping.

## Interactive sandbox controls

After loading the world from the character screen:

- `W`, `A`, `S`, `D` move the gravity-driven player relative to its facing direction.
- Hold the right mouse button and drag horizontally to turn, or vertically to look up and down.
- Scroll towards the player to enter first person; the player model is hidden at minimum distance. Scroll out for the chase camera.
- Left-click an enemy to raycast-select it. Only the selected enemy displays its object ID.
- Enemy physics bodies block the player and use the same bounds as selection.

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

Three launchable sandbox projects are available under the solution's `Sandbox` folder:

- `widgecraft_ui_sandbox`: scene/UI switching, camera follow, enemy selection, collision and minimap.
- `widgecraft_terrain_sandbox`: gradual seeded terrain, WASD movement, heightfield gravity and `Space` jumping.
- `widgecraft_batch_sandbox`: 1,000 OBJ/STL objects with SDF ID labels, queue/memory statistics and a draw-distance slider. Use WASD and right-drag to move the camera.

To run one in Visual Studio, right-click the sandbox project, choose **Set as Startup Project**, then press `F5` (debug) or `Ctrl+F5` (without the debugger).

## Build

```bat
cmake --preset vs2022-win32-release
cmake --build --preset build-release --parallel
```
