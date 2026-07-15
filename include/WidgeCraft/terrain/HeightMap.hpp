#pragma once

#include "WidgeCraft/primitives/Types.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace WidgeCraft {

    class HeightMap {
    public:
        HeightMap() = default;
        HeightMap(
            std::size_t width,
            std::size_t depth,
            float cellSize,
            std::vector<float> heights,
            const Vec3& origin = {});

        static HeightMap loadPgm(
            const std::filesystem::path& path,
            float cellSize = 1.0f,
            float heightScale = 1.0f,
            float heightOffset = 0.0f,
            const Vec3& origin = {});

        static HeightMap generateGradual(
            std::size_t width,
            std::size_t depth,
            float cellSize,
            float maximumNeighbourStep,
            std::uint32_t seed = 0,
            int smoothingPasses = 3,
            const Vec3& origin = {});

        bool empty() const { return m_heights.empty(); }
        std::size_t getWidth() const { return m_width; }
        std::size_t getDepth() const { return m_depth; }
        float getCellSize() const { return m_cellSize; }
        Vec3 getOrigin() const { return m_origin; }

        float getHeight(std::size_t x, std::size_t z) const;
        void setHeight(std::size_t x, std::size_t z, float height);
        const std::vector<float>& getHeights() const { return m_heights; }

        float getWorldWidth() const;
        float getWorldDepth() const;
        bool contains(float worldX, float worldZ) const;

        // Samples the exact alternating-triangle surface used by Terrain.
        std::optional<float> sampleHeight(
            float worldX,
            float worldZ) const;
        std::optional<Vec3> sampleNormal(
            float worldX,
            float worldZ) const;

    private:
        std::size_t index(std::size_t x, std::size_t z) const;
        void validate() const;

        std::size_t m_width = 0;
        std::size_t m_depth = 0;
        float m_cellSize = 1.0f;
        Vec3 m_origin{};
        std::vector<float> m_heights;
    };

} // namespace WidgeCraft
