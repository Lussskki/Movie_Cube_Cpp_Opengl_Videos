#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace
{
constexpr unsigned int SCR_WIDTH = 1000;
constexpr unsigned int SCR_HEIGHT = 600;
constexpr int DEFAULT_VIDEO_WIDTH = 256;
constexpr int DEFAULT_VIDEO_HEIGHT = 256;
constexpr int FACE_COUNT = 6;
constexpr float FIXED_CUBE_SCALE = 2.4f;
const std::string AUDIO_VIDEO_PATH = "C:\\Users\\Luka\\Videos\\2025-09-26 23-28-31.mp4";

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D videoTexture;
uniform float brightness;

void main()
{
    vec3 color = texture(videoTexture, TexCoord).rgb;
    FragColor = vec4(color * brightness, 1.0);
}
)";

struct VideoFrame
{
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;
};

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

GLuint CompileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
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

GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource)
{
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

class Camera
{
public:
    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

    float Yaw = -90.0f;
    float Pitch = 0.0f;
    float MovementSpeed = 4.0f;
    float MouseSensitivity = 0.1f;

    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(GLFWwindow* window, float deltaTime)
    {
        const float velocity = MovementSpeed * deltaTime;
        const glm::vec3 right = glm::normalize(glm::cross(Front, Up));

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) Position += Front * velocity;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) Position -= Front * velocity;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) Position -= right * velocity;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) Position += right * velocity;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) Position += Up * velocity;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) Position -= Up * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;

        UpdateVectors();
    }

private:
    void UpdateVectors()
    {
        glm::vec3 front;
        front.x = std::cos(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));
        front.y = std::sin(glm::radians(Pitch));
        front.z = std::sin(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));
        Front = glm::normalize(front);
    }
};

class FaceVideoTextures
{
public:
    FaceVideoTextures()
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

    ~FaceVideoTextures()
    {
        glDeleteTextures(FACE_COUNT, textures.data());
    }

    void Update(float time)
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

    void Bind(int face) const
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[face]);
    }

private:
    void LoadFrameLists()
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

    bool LoadFrameForTime(int face, float time, VideoFrame& frame) const
    {
        const auto& paths = framePaths[face].empty() ? framePaths[0] : framePaths[face];
        if (paths.empty()) return false;

        const int fps = 24;
        const std::size_t frameIndex = static_cast<std::size_t>(time * fps) % paths.size();
        return LoadPpmFrame(paths[frameIndex], frame);
    }

    void GenerateFallbackFrame(int face, float time, VideoFrame& frame) const
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

    void UploadFrame(int face, const VideoFrame& frame)
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

    std::array<GLuint, FACE_COUNT> textures{};
    std::array<glm::ivec2, FACE_COUNT> textureSizes{};
    std::array<std::vector<std::filesystem::path>, FACE_COUNT> framePaths;
    VideoFrame fallbackFrame;
};

class AudioPlayer
{
public:
    void Start(const std::string& videoPath)
    {
#ifdef _WIN32
        if (started || !std::filesystem::exists(videoPath)) return;

        std::string command = "ffplay -nodisp -loop 0 -loglevel quiet -autoexit \"" + videoPath + "\"";
        STARTUPINFOA startupInfo{};
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

    void Stop()
    {
#ifdef _WIN32
        if (!started) return;

        TerminateProcess(processInfo.hProcess, 0);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        processInfo = {};
        started = false;
#endif
    }

    ~AudioPlayer()
    {
        Stop();
    }

private:
#ifdef _WIN32
    PROCESS_INFORMATION processInfo{};
#endif
    bool started = false;
};

class Cube
{
public:
    explicit Cube(const glm::vec3& pos)
        : position(pos)
    {
        constexpr float cubeVertices[] = {
            -0.5f,-0.5f,-0.5f,  0.0f,1.0f,
             0.5f,-0.5f,-0.5f,  1.0f,1.0f,
             0.5f, 0.5f,-0.5f,  1.0f,0.0f,
             0.5f, 0.5f,-0.5f,  1.0f,0.0f,
            -0.5f, 0.5f,-0.5f,  0.0f,0.0f,
            -0.5f,-0.5f,-0.5f,  0.0f,1.0f,

            -0.5f,-0.5f, 0.5f,  0.0f,1.0f,
             0.5f,-0.5f, 0.5f,  1.0f,1.0f,
             0.5f, 0.5f, 0.5f,  1.0f,0.0f,
             0.5f, 0.5f, 0.5f,  1.0f,0.0f,
            -0.5f, 0.5f, 0.5f,  0.0f,0.0f,
            -0.5f,-0.5f, 0.5f,  0.0f,1.0f,

            -0.5f, 0.5f, 0.5f,  1.0f,1.0f,
            -0.5f, 0.5f,-0.5f,  1.0f,0.0f,
            -0.5f,-0.5f,-0.5f,  0.0f,0.0f,
            -0.5f,-0.5f,-0.5f,  0.0f,0.0f,
            -0.5f,-0.5f, 0.5f,  0.0f,1.0f,
            -0.5f, 0.5f, 0.5f,  1.0f,1.0f,

             0.5f, 0.5f, 0.5f,  1.0f,1.0f,
             0.5f, 0.5f,-0.5f,  1.0f,0.0f,
             0.5f,-0.5f,-0.5f,  0.0f,0.0f,
             0.5f,-0.5f,-0.5f,  0.0f,0.0f,
             0.5f,-0.5f, 0.5f,  0.0f,1.0f,
             0.5f, 0.5f, 0.5f,  1.0f,1.0f,

            -0.5f,-0.5f,-0.5f,  0.0f,0.0f,
             0.5f,-0.5f,-0.5f,  1.0f,0.0f,
             0.5f,-0.5f, 0.5f,  1.0f,1.0f,
             0.5f,-0.5f, 0.5f,  1.0f,1.0f,
            -0.5f,-0.5f, 0.5f,  0.0f,1.0f,
            -0.5f,-0.5f,-0.5f,  0.0f,0.0f,

            -0.5f, 0.5f,-0.5f,  0.0f,0.0f,
             0.5f, 0.5f,-0.5f,  1.0f,0.0f,
             0.5f, 0.5f, 0.5f,  1.0f,1.0f,
             0.5f, 0.5f, 0.5f,  1.0f,1.0f,
            -0.5f, 0.5f, 0.5f,  0.0f,1.0f,
            -0.5f, 0.5f,-0.5f,  0.0f,0.0f
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    ~Cube()
    {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
    }

    void Update(float time)
    {
        videoTextures.Update(time);
    }

    void Draw(GLuint shaderProgram) const
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(FIXED_CUBE_SCALE));

        const GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        const GLint brightnessLoc = glGetUniformLocation(shaderProgram, "brightness");
        if (modelLoc >= 0) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(VAO);
        for (int face = 0; face < FACE_COUNT; ++face)
        {
            if (brightnessLoc >= 0) glUniform1f(brightnessLoc, 1.05f + face * 0.03f);
            videoTextures.Bind(face);
            glDrawArrays(GL_TRIANGLES, face * 6, 6);
        }
        glBindVertexArray(0);
    }

    glm::vec3 position;

private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    FaceVideoTextures videoTextures;
};

class Environment
{
public:
    Environment()
    {
        if (!glfwInit())
        {
            std::cerr << "GLFW init failed\n";
            return;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Movie Cube", nullptr, nullptr);
        if (!window)
        {
            std::cerr << "Failed to create GLFW window\n";
            glfwTerminate();
            return;
        }

        glfwMakeContextCurrent(window);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
        glfwSetCursorPosCallback(window, MouseCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK)
        {
            std::cerr << "GLEW init failed\n";
            return;
        }

        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glEnable(GL_DEPTH_TEST);

        shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "videoTexture"), 0);

        ready = true;
    }

    ~Environment()
    {
        if (shaderProgram != 0) glDeleteProgram(shaderProgram);
        glfwTerminate();
    }

    bool IsReady() const
    {
        return ready;
    }

    int Run()
    {
        audioPlayer.Start(AUDIO_VIDEO_PATH);

        Cube cube(glm::vec3(0.0f, 0.0f, -2.0f));
        float lastFrame = 0.0f;

        while (!glfwWindowShouldClose(window))
        {
            const float currentFrame = static_cast<float>(glfwGetTime());
            const float deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            ProcessInput(deltaTime);
            cube.Update(currentFrame);
            Render(cube);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        return 0;
    }

private:
    static void FramebufferSizeCallback(GLFWwindow*, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    static void MouseCallback(GLFWwindow* window, double xpos, double ypos)
    {
        Environment* env = static_cast<Environment*>(glfwGetWindowUserPointer(window));
        if (env == nullptr) return;

        if (env->firstMouse)
        {
            env->lastX = static_cast<float>(xpos);
            env->lastY = static_cast<float>(ypos);
            env->firstMouse = false;
        }

        const float xoffset = static_cast<float>(xpos) - env->lastX;
        const float yoffset = env->lastY - static_cast<float>(ypos);
        env->lastX = static_cast<float>(xpos);
        env->lastY = static_cast<float>(ypos);

        env->camera.ProcessMouseMovement(xoffset, yoffset);
    }

    void ProcessInput(float deltaTime)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        camera.ProcessKeyboard(window, deltaTime);
    }

    void Render(const Cube& cube) const
    {
        glClearColor(0.03f, 0.04f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT),
            0.1f,
            100.0f
        );

        const GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        const GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        if (viewLoc >= 0) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        if (projectionLoc >= 0) glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        cube.Draw(shaderProgram);
    }

    GLFWwindow* window = nullptr;
    GLuint shaderProgram = 0;
    Camera camera;
    AudioPlayer audioPlayer;

    bool ready = false;
    bool firstMouse = true;
    float lastX = SCR_WIDTH * 0.5f;
    float lastY = SCR_HEIGHT * 0.5f;
};
}

int RunCubeScene()
{
    Environment environment;
    if (!environment.IsReady())
    {
        return -1;
    }

    return environment.Run();
}
