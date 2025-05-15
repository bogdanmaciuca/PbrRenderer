#pragma once

#include "pch.h"

class Platform {
public:
    Platform(const string& wndName, i32 wndWidth, i32 wndHeight);
    ~Platform();
    bool ShouldClose() const;
    SDL_Window* GetSDLWindow() const;
    void HandleEvents();
    bool KeyIsDown(u32 key) const;
    float GetMouseX() const;
    float GetMouseY() const;
private:
    bool m_shouldClose = false;
    SDL_Window* m_pWindow;
    const bool* m_pKeyboardState;
    float m_mouseX = 0.0f, m_mouseY = 0.0f;
};


