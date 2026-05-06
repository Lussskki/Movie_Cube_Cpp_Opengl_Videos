#include "headers/Camera.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard(GLFWwindow* window, float deltaTime)
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

void Camera::ProcessMouseMovement(float xoffset, float yoffset)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;

    UpdateVectors();
}

void Camera::UpdateVectors()
{
    glm::vec3 front;
    front.x = std::cos(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));
    front.y = std::sin(glm::radians(Pitch));
    front.z = std::sin(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));
    Front = glm::normalize(front);
}
