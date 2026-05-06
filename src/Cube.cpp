#include "headers/Camera.h"
#include "headers/ShaderProgram.h"
#include "headers/Video.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <iostream>
#include <string>

namespace
{
constexpr unsigned int SCR_WIDTH = 1000;
constexpr unsigned int SCR_HEIGHT = 600;
constexpr float FIXED_CUBE_SCALE = 2.4f;
const std::string AUDIO_VIDEO_PATH = "C:\\Users\\Luka\\Videos\\2025-09-26 23-28-31.mp4";

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

private:
    glm::vec3 position;
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

        shaderProgram = CreateShaderProgramFromFiles(
            "src/shaders/video_cube.vert",
            "src/shaders/video_cube.frag"
        );
        if (shaderProgram == 0) return;

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
