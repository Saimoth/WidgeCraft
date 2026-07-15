#include "WidgeCraft/model/ModelLoader.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace WidgeCraft {

    namespace {
        std::string lowercase(std::string value) {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char character) {
                    return static_cast<char>(std::tolower(character));
                });
            return value;
        }

        std::uint32_t resolveObjIndex(
            const std::string& token,
            std::size_t vertexCount) {

            const std::size_t slash = token.find('/');
            const std::string positionToken = token.substr(0, slash);
            if (positionToken.empty()) {
                throw std::runtime_error(
                    "OBJ face is missing a position index");
            }

            const int rawIndex = std::stoi(positionToken);
            if (rawIndex == 0) {
                throw std::runtime_error("OBJ indices are one-based");
            }

            const long long resolved = rawIndex > 0
                ? static_cast<long long>(rawIndex - 1)
                : static_cast<long long>(vertexCount) + rawIndex;
            if (resolved < 0
                || resolved >= static_cast<long long>(vertexCount)) {
                throw std::runtime_error(
                    "OBJ face references a vertex outside the model");
            }
            return static_cast<std::uint32_t>(resolved);
        }

        std::vector<unsigned char> readBinaryFile(
            const std::filesystem::path& path) {

            std::ifstream stream(path, std::ios::binary | std::ios::ate);
            if (!stream) {
                throw std::runtime_error(
                    "Unable to open model file: " + path.string());
            }

            const std::streamsize size = stream.tellg();
            if (size < 0) {
                throw std::runtime_error(
                    "Unable to determine model file size: " + path.string());
            }
            stream.seekg(0, std::ios::beg);

            std::vector<unsigned char> bytes(
                static_cast<std::size_t>(size));
            if (!bytes.empty()
                && !stream.read(
                    reinterpret_cast<char*>(bytes.data()),
                    size)) {
                throw std::runtime_error(
                    "Unable to read model file: " + path.string());
            }
            return bytes;
        }

        std::uint32_t readUint32(
            const std::vector<unsigned char>& bytes,
            std::size_t offset) {

            std::uint32_t value = 0;
            std::memcpy(&value, bytes.data() + offset, sizeof(value));
            return value;
        }

        float readFloat(
            const std::vector<unsigned char>& bytes,
            std::size_t offset) {

            float value = 0.0f;
            std::memcpy(&value, bytes.data() + offset, sizeof(value));
            return value;
        }

        bool isBinaryStl(const std::vector<unsigned char>& bytes) {
            if (bytes.size() < 84U) {
                return false;
            }
            const std::uint32_t triangleCount = readUint32(bytes, 80U);
            const std::uint64_t expectedSize = 84ULL
                + static_cast<std::uint64_t>(triangleCount) * 50ULL;
            return expectedSize == bytes.size();
        }
    } // namespace

    Mesh3D ModelLoader::load(const std::filesystem::path& path) {
        const std::string extension = lowercase(path.extension().string());
        if (extension == ".obj") {
            return loadObj(path);
        }
        if (extension == ".stl") {
            return loadStl(path);
        }
        throw std::invalid_argument(
            "Unsupported model format '" + extension
            + "'; expected .obj or .stl");
    }

    Mesh3D ModelLoader::loadObj(const std::filesystem::path& path) {
        std::ifstream stream(path);
        if (!stream) {
            throw std::runtime_error(
                "Unable to open OBJ file: " + path.string());
        }

        Mesh3D mesh;
        std::string line;
        std::size_t lineNumber = 0;
        while (std::getline(stream, line)) {
            ++lineNumber;
            std::istringstream parser(line);
            std::string command;
            parser >> command;
            if (command.empty() || command[0] == '#') {
                continue;
            }

            if (command == "v") {
                Vec3 vertex;
                if (!(parser >> vertex.x >> vertex.y >> vertex.z)) {
                    throw std::runtime_error(
                        "Invalid OBJ vertex at line "
                        + std::to_string(lineNumber));
                }
                mesh.vertices.push_back(vertex);
                continue;
            }

            if (command == "f") {
                std::vector<std::uint32_t> face;
                std::string token;
                while (parser >> token) {
                    face.push_back(resolveObjIndex(
                        token,
                        mesh.vertices.size()));
                }
                if (face.size() < 3U) {
                    throw std::runtime_error(
                        "OBJ face has fewer than three vertices at line "
                        + std::to_string(lineNumber));
                }
                for (std::size_t index = 1;
                     index + 1U < face.size();
                     ++index) {
                    mesh.indices.push_back(face[0]);
                    mesh.indices.push_back(face[index]);
                    mesh.indices.push_back(face[index + 1U]);
                }
            }
        }

        if (!mesh.isValid()) {
            throw std::runtime_error(
                "OBJ contains no valid triangle mesh: " + path.string());
        }
        return mesh;
    }

    Mesh3D ModelLoader::loadStl(const std::filesystem::path& path) {
        const std::vector<unsigned char> bytes = readBinaryFile(path);
        Mesh3D mesh;

        if (isBinaryStl(bytes)) {
            const std::uint32_t triangleCount = readUint32(bytes, 80U);
            mesh.vertices.reserve(
                static_cast<std::size_t>(triangleCount) * 3U);
            mesh.indices.reserve(
                static_cast<std::size_t>(triangleCount) * 3U);

            for (std::uint32_t triangle = 0;
                 triangle < triangleCount;
                 ++triangle) {
                const std::size_t record = 84U
                    + static_cast<std::size_t>(triangle) * 50U;
                for (std::size_t vertex = 0; vertex < 3U; ++vertex) {
                    const std::size_t offset = record + 12U + vertex * 12U;
                    mesh.vertices.push_back({
                        readFloat(bytes, offset),
                        readFloat(bytes, offset + 4U),
                        readFloat(bytes, offset + 8U)
                    });
                    mesh.indices.push_back(
                        static_cast<std::uint32_t>(mesh.indices.size()));
                }
            }
        } else {
            const std::string text(bytes.begin(), bytes.end());
            std::istringstream stream(text);
            std::string line;
            while (std::getline(stream, line)) {
                std::istringstream parser(line);
                std::string command;
                parser >> command;
                if (lowercase(command) != "vertex") {
                    continue;
                }
                Vec3 vertex;
                if (!(parser >> vertex.x >> vertex.y >> vertex.z)) {
                    throw std::runtime_error(
                        "Invalid vertex in ASCII STL: " + path.string());
                }
                mesh.vertices.push_back(vertex);
                mesh.indices.push_back(
                    static_cast<std::uint32_t>(mesh.indices.size()));
            }
        }

        if (!mesh.isValid()) {
            throw std::runtime_error(
                "STL contains no valid triangles: " + path.string());
        }
        return mesh;
    }

    Mesh2D ModelLoader::load2D(
        const std::filesystem::path& path,
        MeshProjectionPlane plane) {
        return projectTo2D(load(path), plane);
    }

    Mesh2D ModelLoader::projectTo2D(
        const Mesh3D& mesh,
        MeshProjectionPlane plane) {

        if (!mesh.isValid()) {
            throw std::invalid_argument(
                "Cannot project an invalid 3D mesh");
        }

        Mesh2D result;
        result.indices = mesh.indices;
        result.vertices.reserve(mesh.vertices.size());
        for (const Vec3& vertex : mesh.vertices) {
            switch (plane) {
            case MeshProjectionPlane::XZ:
                result.vertices.push_back({ vertex.x, vertex.z });
                break;
            case MeshProjectionPlane::YZ:
                result.vertices.push_back({ vertex.y, vertex.z });
                break;
            case MeshProjectionPlane::XY:
            default:
                result.vertices.push_back({ vertex.x, vertex.y });
                break;
            }
        }
        return result;
    }

} // namespace WidgeCraft
