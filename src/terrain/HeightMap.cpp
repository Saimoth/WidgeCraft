#include "WidgeCraft/terrain/HeightMap.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>

namespace WidgeCraft {

    namespace {
        std::string readPgmToken(std::istream& stream) {
            std::string token;
            char character = 0;

            while (stream.get(character)) {
                if (character == '#') {
                    stream.ignore(
                        std::numeric_limits<std::streamsize>::max(),
                        '\n');
                    continue;
                }
                if (!std::isspace(
                        static_cast<unsigned char>(character))) {
                    token.push_back(character);
                    break;
                }
            }

            while (stream.get(character)) {
                if (character == '#') {
                    stream.ignore(
                        std::numeric_limits<std::streamsize>::max(),
                        '\n');
                    break;
                }
                if (std::isspace(
                        static_cast<unsigned char>(character))) {
                    break;
                }
                token.push_back(character);
            }
            return token;
        }
    } // namespace

    HeightMap::HeightMap(
        std::size_t width,
        std::size_t depth,
        float cellSize,
        std::vector<float> heights,
        const Vec3& origin)
        : m_width(width)
        , m_depth(depth)
        , m_cellSize(cellSize)
        , m_origin(origin)
        , m_heights(std::move(heights)) {
        validate();
    }

    HeightMap HeightMap::loadPgm(
        const std::filesystem::path& path,
        float cellSize,
        float heightScale,
        float heightOffset,
        const Vec3& origin) {

        std::ifstream stream(path, std::ios::binary);
        if (!stream) {
            throw std::runtime_error(
                "Unable to open PGM heightmap: " + path.string());
        }

        const std::string magic = readPgmToken(stream);
        if (magic != "P2" && magic != "P5") {
            throw std::runtime_error(
                "Heightmap must be a P2 or P5 PGM file: "
                + path.string());
        }

        const std::size_t width = static_cast<std::size_t>(
            std::stoul(readPgmToken(stream)));
        const std::size_t depth = static_cast<std::size_t>(
            std::stoul(readPgmToken(stream)));
        const int maximumValue = std::stoi(readPgmToken(stream));
        if (width < 2U || depth < 2U || maximumValue <= 0) {
            throw std::runtime_error(
                "Invalid PGM heightmap dimensions or range");
        }
        if (magic == "P5" && maximumValue > 255) {
            throw std::runtime_error(
                "16-bit binary PGM heightmaps are not yet supported");
        }

        std::vector<float> heights(width * depth);
        for (std::size_t index = 0; index < heights.size(); ++index) {
            int sample = 0;
            if (magic == "P2") {
                const std::string token = readPgmToken(stream);
                if (token.empty()) {
                    throw std::runtime_error(
                        "PGM heightmap ended before all samples were read");
                }
                sample = std::stoi(token);
            } else {
                const int value = stream.get();
                if (value == std::char_traits<char>::eof()) {
                    throw std::runtime_error(
                        "PGM heightmap ended before all samples were read");
                }
                sample = value;
            }

            const float normalizedValue = std::clamp(
                static_cast<float>(sample)
                    / static_cast<float>(maximumValue),
                0.0f,
                1.0f);
            heights[index] = heightOffset
                + normalizedValue * heightScale;
        }
        return HeightMap(
            width,
            depth,
            cellSize,
            std::move(heights),
            origin);
    }

    HeightMap HeightMap::generateGradual(
        std::size_t width,
        std::size_t depth,
        float cellSize,
        float maximumNeighbourStep,
        std::uint32_t seed,
        int smoothingPasses,
        const Vec3& origin) {

        if (width < 2U || depth < 2U) {
            throw std::invalid_argument(
                "A heightmap requires at least two vertices per axis");
        }
        const float step = std::max(maximumNeighbourStep, 0.001f);
        std::mt19937 generator(seed);
        std::uniform_real_distribution<float> offset(-step, step);
        std::vector<float> heights(width * depth, 0.0f);

        for (std::size_t z = 0; z < depth; ++z) {
            for (std::size_t x = 0; x < width; ++x) {
                float lower = -std::numeric_limits<float>::max();
                float upper = std::numeric_limits<float>::max();
                float average = 0.0f;
                int neighbourCount = 0;

                if (x > 0U) {
                    const float left = heights[z * width + x - 1U];
                    lower = std::max(lower, left - step);
                    upper = std::min(upper, left + step);
                    average += left;
                    ++neighbourCount;
                }
                if (z > 0U) {
                    const float below = heights[(z - 1U) * width + x];
                    lower = std::max(lower, below - step);
                    upper = std::min(upper, below + step);
                    average += below;
                    ++neighbourCount;
                }

                float value = neighbourCount > 0
                    ? average / static_cast<float>(neighbourCount)
                        + offset(generator)
                    : offset(generator);
                if (lower <= upper) {
                    value = std::clamp(value, lower, upper);
                }
                heights[z * width + x] = value;
            }
        }

        std::vector<float> smoothed(heights.size());
        const int passes = std::clamp(smoothingPasses, 0, 32);
        for (int pass = 0; pass < passes; ++pass) {
            for (std::size_t z = 0; z < depth; ++z) {
                for (std::size_t x = 0; x < width; ++x) {
                    float sum = 0.0f;
                    int count = 0;
                    for (int dz = -1; dz <= 1; ++dz) {
                        const int sampleZ = static_cast<int>(z) + dz;
                        if (sampleZ < 0
                            || sampleZ >= static_cast<int>(depth)) {
                            continue;
                        }
                        for (int dx = -1; dx <= 1; ++dx) {
                            const int sampleX = static_cast<int>(x) + dx;
                            if (sampleX < 0
                                || sampleX >= static_cast<int>(width)) {
                                continue;
                            }
                            sum += heights[
                                static_cast<std::size_t>(sampleZ) * width
                                + static_cast<std::size_t>(sampleX)];
                            ++count;
                        }
                    }
                    smoothed[z * width + x] = sum
                        / static_cast<float>(count);
                }
            }
            heights.swap(smoothed);
        }

        float averageHeight = 0.0f;
        for (float height : heights) {
            averageHeight += height;
        }
        averageHeight /= static_cast<float>(heights.size());
        for (float& height : heights) {
            height -= averageHeight;
        }

        // Normalized box smoothing is normally non-expansive, but scale the
        // final field defensively so the public neighbour limit is strict at
        // image boundaries as well as in the interior.
        float largestNeighbourStep = 0.0f;
        for (std::size_t z = 0; z < depth; ++z) {
            for (std::size_t x = 0; x < width; ++x) {
                const float value = heights[z * width + x];
                if (x + 1U < width) {
                    largestNeighbourStep = std::max(
                        largestNeighbourStep,
                        std::abs(value - heights[z * width + x + 1U]));
                }
                if (z + 1U < depth) {
                    largestNeighbourStep = std::max(
                        largestNeighbourStep,
                        std::abs(value - heights[(z + 1U) * width + x]));
                }
            }
        }
        if (largestNeighbourStep > step) {
            const float scale = step / largestNeighbourStep;
            for (float& height : heights) {
                height *= scale;
            }
        }

        return HeightMap(
            width,
            depth,
            cellSize,
            std::move(heights),
            origin);
    }

    float HeightMap::getHeight(std::size_t x, std::size_t z) const {
        if (x >= m_width || z >= m_depth) {
            throw std::out_of_range("Heightmap vertex is out of range");
        }
        return m_heights[index(x, z)];
    }

    void HeightMap::setHeight(
        std::size_t x,
        std::size_t z,
        float height) {
        if (x >= m_width || z >= m_depth) {
            throw std::out_of_range("Heightmap vertex is out of range");
        }
        m_heights[index(x, z)] = height;
    }

    float HeightMap::getWorldWidth() const {
        return m_width > 0U
            ? static_cast<float>(m_width - 1U) * m_cellSize
            : 0.0f;
    }

    float HeightMap::getWorldDepth() const {
        return m_depth > 0U
            ? static_cast<float>(m_depth - 1U) * m_cellSize
            : 0.0f;
    }

    bool HeightMap::contains(float worldX, float worldZ) const {
        return worldX >= m_origin.x
            && worldZ >= m_origin.z
            && worldX <= m_origin.x + getWorldWidth()
            && worldZ <= m_origin.z + getWorldDepth();
    }

    std::optional<float> HeightMap::sampleHeight(
        float worldX,
        float worldZ) const {

        if (!contains(worldX, worldZ) || empty()) {
            return std::nullopt;
        }

        const float gridX = (worldX - m_origin.x) / m_cellSize;
        const float gridZ = (worldZ - m_origin.z) / m_cellSize;
        const std::size_t cellX = std::min(
            static_cast<std::size_t>(std::floor(gridX)),
            m_width - 2U);
        const std::size_t cellZ = std::min(
            static_cast<std::size_t>(std::floor(gridZ)),
            m_depth - 2U);
        const float x = std::clamp(
            gridX - static_cast<float>(cellX),
            0.0f,
            1.0f);
        const float z = std::clamp(
            gridZ - static_cast<float>(cellZ),
            0.0f,
            1.0f);

        const float h00 = getHeight(cellX, cellZ);
        const float h10 = getHeight(cellX + 1U, cellZ);
        const float h01 = getHeight(cellX, cellZ + 1U);
        const float h11 = getHeight(cellX + 1U, cellZ + 1U);

        float height = 0.0f;
        if ((cellX + cellZ) % 2U == 0U) {
            if (x >= z) {
                height = h00 * (1.0f - x)
                    + h10 * (x - z)
                    + h11 * z;
            } else {
                height = h00 * (1.0f - z)
                    + h11 * x
                    + h01 * (z - x);
            }
        } else if (x + z <= 1.0f) {
            height = h00 * (1.0f - x - z)
                + h10 * x
                + h01 * z;
        } else {
            height = h10 * (1.0f - z)
                + h11 * (x + z - 1.0f)
                + h01 * (1.0f - x);
        }
        return m_origin.y + height;
    }

    std::optional<Vec3> HeightMap::sampleNormal(
        float worldX,
        float worldZ) const {

        const auto center = sampleHeight(worldX, worldZ);
        if (!center) {
            return std::nullopt;
        }

        const float offset = std::max(m_cellSize * 0.25f, 0.001f);
        const float left = sampleHeight(worldX - offset, worldZ)
            .value_or(*center);
        const float right = sampleHeight(worldX + offset, worldZ)
            .value_or(*center);
        const float below = sampleHeight(worldX, worldZ - offset)
            .value_or(*center);
        const float above = sampleHeight(worldX, worldZ + offset)
            .value_or(*center);
        return normalized(Vec3{
            left - right,
            offset * 2.0f,
            below - above
        });
    }

    std::size_t HeightMap::index(
        std::size_t x,
        std::size_t z) const {
        return z * m_width + x;
    }

    void HeightMap::validate() const {
        if (m_width < 2U || m_depth < 2U) {
            throw std::invalid_argument(
                "A heightmap requires at least two vertices per axis");
        }
        if (m_cellSize <= 0.0f) {
            throw std::invalid_argument(
                "Heightmap cell size must be positive");
        }
        if (m_heights.size() != m_width * m_depth) {
            throw std::invalid_argument(
                "Heightmap sample count does not match its dimensions");
        }
    }

} // namespace WidgeCraft
