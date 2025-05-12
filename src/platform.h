#pragma once

#include "pch.h"

class Platform {
public:
    Platform(const string& wndName, i32 wndWidth, i32 wndHeight);
    ~Platform();
    bool ShouldClose();
    SDL_Window* GetSDLWindow();
    void HandleEvents();
private:
    bool m_shouldClose = false;
    SDL_Window* m_pWindow;
};


