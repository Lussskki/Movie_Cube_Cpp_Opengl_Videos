#include "headers/Video.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace
{
constexpr int DEFAULT_VIDEO_WIDTH = 256;
constexpr int DEFAULT_VIDEO_HEIGHT = 256;

std::string ReadToken(std::ifstream& file)
{
    std::string token;
    while (file >> token)
    {
        if (!token.empty() && token[0] == '#')
        {
            std::string ignored;
            std::getline(file, ignored);
            continue;
        }
        return token;
    }

    return "";
}
}

bool LoadPpmFrame(const std::filesystem::path& path, VideoFrame& frame)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    const std::string magic = ReadToken(file);
    if (magic != "P6") return false;

    const int width = std::stoi(ReadToken(file));
    const int height = std::stoi(ReadToken(file));
    const int maxValue = std::stoi(ReadToken(file));
    if (width <= 0 || height <= 0 || maxValue != 255) return false;

    file.get();

    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 3);
    file.read(reinterpret_cast<char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
    if (file.gcount() != static_cast<std::streamsize>(pixels.size())) return false;

    frame.width = width;
    frame.height = height;
    frame.pixels = std::move(pixels);
    return true;
}

FaceVideoTextures::FaceVideoTextures()
{
    LoadFrameLists();
    glGenTextures(FACE_COUNT, textures.data());

    for (int face = 0; face < FACE_COUNT; ++face)
    {
        glBindTexture(GL_TEXTURE_2D, textures[face]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GenerateFallbackFrame(face, 0.0f, fallbackFrame);
        UploadFrame(face, fallbackFrame);
    }
}

FaceVideoTextures::~FaceVideoTextures()
{
    glDeleteTextures(FACE_COUNT, textures.data());
}

void FaceVideoTextures::Update(float time)
{
    for (int face = 0; face < FACE_COUNT; ++face)
    {
        VideoFrame frame;
        if (LoadFrameForTime(face, time, frame))
        {
            UploadFrame(face, frame);
        }
        else
        {
            GenerateFallbackFrame(face, time, fallbackFrame);
            UploadFrame(face, fallbackFrame);
        }
    }
}

void FaceVideoTextures::Bind(int face) const
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[face]);
}

void FaceVideoTextures::LoadFrameLists()
{
    const std::filesystem::path basePath = "assets/video_faces";
    for (int face = 0; face < FACE_COUNT; ++face)
    {
        const std::filesystem::path facePath = basePath / ("face" + std::to_string(face));
        if (!std::filesystem::exists(facePath)) continue;

        for (const auto& entry : std::filesystem::directory_iterator(facePath))
        {
            if (!entry.is_regular_file()) continue;

            std::filesystem::path path = entry.path();
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            if (extension == ".ppm")
            {
                framePaths[face].push_back(path);
            }
        }

        std::sort(framePaths[face].begin(), framePaths[face].end());
        if (!framePaths[face].empty())
        {
            std::cout << "Loaded " << framePaths[face].size()
                      << " frame paths for cube face " << face << '\n';
        }
    }
}

bool FaceVideoTextures::LoadFrameForTime(int face, float time, VideoFrame& frame) const
{
    const auto& paths = framePaths[face].empty() ? framePaths[0] : framePaths[face];
    if (paths.empty()) return false;

    const int fps = 24;
    const std::size_t frameIndex = static_cast<std::size_t>(time * fps) % paths.size();
    return LoadPpmFrame(paths[frameIndex], frame);
}

void FaceVideoTextures::GenerateFallbackFrame(int face, float time, VideoFrame& frame) const
{
    frame.width = DEFAULT_VIDEO_WIDTH;
    frame.height = DEFAULT_VIDEO_HEIGHT;
    frame.pixels.resize(static_cast<std::size_t>(frame.width) * frame.height * 3);

    const float t = time * (0.7f + face * 0.08f);
    const float stripe = std::sin(t * 3.0f);

    for (int y = 0; y < frame.height; ++y)
    {
        for (int x = 0; x < frame.width; ++x)
        {
            const float u = static_cast<float>(x) / static_cast<float>(frame.width - 1);
            const float v = static_cast<float>(y) / static_cast<float>(frame.height - 1);
            const float wave = 0.5f + 0.5f * std::sin((u * 10.0f) + (v * 7.0f) + t * 4.0f);
            const bool scanline = (y / 4) % 2 == 0;

            std::uint8_t r = static_cast<std::uint8_t>(255.0f * std::fmod(u + face * 0.17f + t * 0.06f, 1.0f));
            std::uint8_t g = static_cast<std::uint8_t>(255.0f * std::fmod(v + face * 0.11f + wave * 0.25f, 1.0f));
            std::uint8_t b = static_cast<std::uint8_t>(255.0f * (0.35f + 0.65f * wave));

            const bool playTriangle =
                x > frame.width / 2 - 24 && x < frame.width / 2 + 32 &&
                std::abs(y - frame.height / 2) < (x - frame.width / 2 + 24);

            const std::size_t index = (static_cast<std::size_t>(y) * frame.width + x) * 3;
            frame.pixels[index + 0] = playTriangle ? 245 : static_cast<std::uint8_t>(r * (scanline ? 1.0f : 0.75f));
            frame.pixels[index + 1] = playTriangle ? 245 : static_cast<std::uint8_t>(g * (0.75f + stripe * 0.08f));
            frame.pixels[index + 2] = playTriangle ? 245 : static_cast<std::uint8_t>(b * (scanline ? 1.0f : 0.75f));
        }
    }
}

void FaceVideoTextures::UploadFrame(int face, const VideoFrame& frame)
{
    if (frame.width <= 0 || frame.height <= 0 || frame.pixels.empty()) return;

    glBindTexture(GL_TEXTURE_2D, textures[face]);
    if (textureSizes[face].x != frame.width || textureSizes[face].y != frame.height)
    {
        textureSizes[face] = glm::ivec2(frame.width, frame.height);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGB,
            frame.width,
            frame.height,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            frame.pixels.data()
        );
    }
    else
    {
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            frame.width,
            frame.height,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            frame.pixels.data()
        );
    }
}

void AudioPlayer::Start(const std::string& videoPath)
{
#ifdef _WIN32
    if (started || !std::filesystem::exists(videoPath)) return;

    std::string command = "ffplay -nodisp -loop 0 -loglevel quiet -autoexit \"" + videoPath + "\"";
    STARTUPINFOA startupInfo{};
    PROCESS_INFORMATION processInfo{};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_HIDE;

    if (CreateProcessA(
            nullptr,
            command.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo))
    {
        processHandle = processInfo.hProcess;
        threadHandle = processInfo.hThread;
        started = true;
    }
    else
    {
        std::cerr << "Could not start ffplay audio. Make sure ffplay is installed.\n";
    }
#else
    (void)videoPath;
#endif
}

void AudioPlayer::Stop()
{
#ifdef _WIN32
    if (!started) return;

    TerminateProcess(static_cast<HANDLE>(processHandle), 0);
    CloseHandle(static_cast<HANDLE>(processHandle));
    CloseHandle(static_cast<HANDLE>(threadHandle));
    processHandle = nullptr;
    threadHandle = nullptr;
    started = false;
#endif
}

AudioPlayer::~AudioPlayer()
{
    Stop();
}
