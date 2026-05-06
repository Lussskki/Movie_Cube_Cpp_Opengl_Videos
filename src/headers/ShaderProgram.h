#pragma once

#include <GL/glew.h>

#include <filesystem>

GLuint CreateShaderProgramFromFiles(
    const std::filesystem::path& vertexPath,
    const std::filesystem::path& fragmentPath
);
