#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

constexpr int FACE_COUNT = 6;

struct VideoFrame
{
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;
};

bool LoadPpmFrame(const std::filesystem::path& path, VideoFrame& frame);

class FaceVideoTextures
{
public:
    FaceVideoTextures();
    ~FaceVideoTextures();

    void Update(float time);
    void Bind(int face) const;

private:
    void LoadFrameLists();
    bool LoadFrameForTime(int face, float time, VideoFrame& frame) const;
    void GenerateFallbackFrame(int face, float time, VideoFrame& frame) const;
    void UploadFrame(int face, const VideoFrame& frame);

    std::array<GLuint, FACE_COUNT> textures{};
    std::array<glm::ivec2, FACE_COUNT> textureSizes{};
    std::array<std::vector<std::filesystem::path>, FACE_COUNT> framePaths;
    VideoFrame fallbackFrame;
};

class AudioPlayer
{
public:
    void Start(const std::string& videoPath);
    void Stop();
    ~AudioPlayer();

private:
#ifdef _WIN32
    void* processHandle = nullptr;
    void* threadHandle = nullptr;
#endif
    bool started = false;
};
