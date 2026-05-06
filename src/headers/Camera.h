#pragma once

#include <glm/glm.hpp>

struct GLFWwindow;

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

    glm::mat4 GetViewMatrix() const;
    void ProcessKeyboard(GLFWwindow* window, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset);

private:
    void UpdateVectors();
};
