#pragma once

#include <string_view>

namespace WidgeCraft {

    class ShaderProgram {
    public:
        ShaderProgram() = default;
        ShaderProgram(std::string_view vertexSource, std::string_view fragmentSource);
        ~ShaderProgram();

        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram& operator=(const ShaderProgram&) = delete;
        ShaderProgram(ShaderProgram&& other) noexcept;
        ShaderProgram& operator=(ShaderProgram&& other) noexcept;

        void create(std::string_view vertexSource, std::string_view fragmentSource);
        void reset();
        void use() const;
        static void stopUsing();

        unsigned int id() const { return m_program; }
        int uniformLocation(const char* name) const;
        explicit operator bool() const { return m_program != 0; }

    private:
        void moveFrom(ShaderProgram&& other) noexcept;

        unsigned int m_program = 0;
    };

} // namespace WidgeCraft
