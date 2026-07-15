#pragma once

#include "WidgeCraft/model/Mesh.hpp"

#include <filesystem>

namespace WidgeCraft {

    enum class MeshProjectionPlane {
        XY,
        XZ,
        YZ
    };

    class ModelLoader {
    public:
        // Loads Wavefront OBJ or STL based on the file extension.
        static Mesh3D load(const std::filesystem::path& path);
        static Mesh3D loadObj(const std::filesystem::path& path);
        static Mesh3D loadStl(const std::filesystem::path& path);

        // OBJ/STL models can also be projected to a batched 2D triangle mesh.
        static Mesh2D load2D(
            const std::filesystem::path& path,
            MeshProjectionPlane plane = MeshProjectionPlane::XY);
        static Mesh2D projectTo2D(
            const Mesh3D& mesh,
            MeshProjectionPlane plane = MeshProjectionPlane::XY);
    };

} // namespace WidgeCraft
