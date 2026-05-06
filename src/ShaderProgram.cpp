#include "headers/ShaderProgram.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
std::string ReadFile(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Could not open shader file: " << path << '\n';
        return "";
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint CompileShader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);
    const char* sourceData = source.c_str();
    glShaderSource(shader, 1, &sourceData, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "Shader compile error: " << infoLog << '\n';
    }

    return shader;
}
}

GLuint CreateShaderProgramFromFiles(
    const std::filesystem::path& vertexPath,
    const std::filesystem::path& fragmentPath
)
{
    const std::string vertexSource = ReadFile(vertexPath);
    const std::string fragmentSource = ReadFile(fragmentPath);
    if (vertexSource.empty() || fragmentSource.empty()) return 0;

    GLuint vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        std::cerr << "Shader link error: " << infoLog << '\n';
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}
