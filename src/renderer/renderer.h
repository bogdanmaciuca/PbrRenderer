#pragma once

#include "../pch.h"

class Renderer {
public:
    Renderer(SDL_Window* pWindow);
    ~Renderer();
private:
    SDL_Window* m_pWindow;
};

