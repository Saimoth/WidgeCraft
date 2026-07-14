#include "WidgeCraft/render/ShaderProgram.hpp"

#include <glad/glad.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace WidgeCraft {

    namespace {
        unsigned int compileShader(unsigned int type, std::string_view source) {
            const unsigned int shader = glCreateShader(type);
            const char* sourceData = source.data();
            const int sourceLength = static_cast<int>(source.size());
            glShaderSource(shader, 1, &sourceData, &sourceLength);
            glCompileShader(shader);

            int status = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
            if (status == GL_FALSE) {
                int logLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
                std::string log(
                    static_cast<std::size_t>(std::max(logLength, 1)),
                    '\0');
                glGetShaderInfoLog(shader, logLength, nullptr, log.data());
                glDeleteShader(shader);
                throw std::runtime_error("Shader compilation failed: " + log);
            }
            return shader;
        }
    } // namespace

    ShaderProgram::ShaderProgram(
        std::string_view vertexSource,
        std::string_view fragmentSource) {
        create(vertexSource, fragmentSource);
    }

    ShaderProgram::~ShaderProgram() {
        reset();
    }

    ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept {
        moveFrom(std::move(other));
    }

    ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
        if (this != &other) {
            reset();
            moveFrom(std::move(other));
        }
        return *this;
    }

    void ShaderProgram::create(
        std::string_view vertexSource,
        std::string_view fragmentSource) {

        reset();
        const unsigned int vertexShader = compileShader(
            GL_VERTEX_SHADER,
            vertexSource);
        const unsigned int fragmentShader = compileShader(
            GL_FRAGMENT_SHADER,
            fragmentSource);

        const unsigned int program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        int status = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            int logLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(
                static_cast<std::size_t>(std::max(logLength, 1)),
                '\0');
            glGetProgramInfoLog(program, logLength, nullptr, log.data());
            glDeleteProgram(program);
            throw std::runtime_error("Shader linking failed: " + log);
        }

        m_program = program;
    }

    void ShaderProgram::reset() {
        if (m_program != 0) {
            glDeleteProgram(m_program);
            m_program = 0;
        }
    }

    void ShaderProgram::use() const {
        glUseProgram(m_program);
    }

    void ShaderProgram::stopUsing() {
        glUseProgram(0);
    }

    int ShaderProgram::uniformLocation(const char* name) const {
        return m_program != 0 ? glGetUniformLocation(m_program, name) : -1;
    }

    void ShaderProgram::moveFrom(ShaderProgram&& other) noexcept {
        m_program = other.m_program;
        other.m_program = 0;
    }

} // namespace WidgeCraft
