#pragma once
#include "pch.h"

class Camera {
public:
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(m_pos, m_pos + m_front, s_up);
    }
    glm::vec3 GetPos() {
        return m_pos;
    }
    void MoveForward(float deltaTime) {
        m_pos += m_front * s_speed * deltaTime;
    }
    void MoveBackword(float deltaTime) {
        m_pos -= m_front * s_speed * deltaTime;
    }
    void MoveLeft(float deltaTime) {
        m_pos -= glm::normalize(glm::cross(m_front, s_up)) * s_speed * deltaTime;
    }
    void MoveRight(float deltaTime) {
        m_pos += glm::normalize(glm::cross(m_front, s_up)) * s_speed * deltaTime;
    }
    void ProcessMouse(float deltaX, float deltaY) {
        m_yaw += deltaX * s_sensitivity;
        m_pitch += deltaY * s_sensitivity;
        m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);

        m_front.x = glm::cos(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
        m_front.y = glm::sin(glm::radians(m_pitch));
        m_front.z = glm::sin(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
        m_front = glm::normalize(m_front);
    }
private:
    glm::vec3 m_pos = glm::vec3(0.0f);
    float m_yaw = 0.0f, m_pitch = 0.0f; // Degrees
    glm::vec3 m_front = glm::vec3(0.0f);
    static constexpr glm::vec3 s_up = glm::vec3(0, 1, 0);
    static constexpr float s_speed = 0.001f;
    static constexpr float s_sensitivity = 0.35f;
};

