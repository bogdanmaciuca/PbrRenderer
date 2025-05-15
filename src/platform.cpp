#include "platform.h"

#include "pch.h"

Platform::Platform(const string& wndName, i32 wndWidth, i32 wndHeight)
    : m_pKeyboardState(SDL_GetKeyboardState(nullptr))
{
    if (SDL_Init(SDL_INIT_VIDEO) == false)
        FatalError("Could not initialize SDL");

    m_pWindow = SDL_CreateWindow(wndName.c_str(), wndWidth, wndHeight, SDL_WINDOW_VULKAN);
    if (m_pWindow == nullptr)
        FatalError("Could not create window");
}

Platform::~Platform() {
    SDL_DestroyWindow(m_pWindow);
}

bool Platform::ShouldClose() const {
    return m_shouldClose;
}

SDL_Window* Platform::GetSDLWindow() const {
    return m_pWindow;
}

void Platform::HandleEvents() {
    SDL_Event evt;
    while (SDL_PollEvent(&evt)) {
        switch (evt.type) {
            case SDL_EVENT_QUIT:
                m_shouldClose = true;
                break;
        }
    }
    SDL_GetMouseState(&m_mouseX, &m_mouseY);
}

bool Platform::KeyIsDown(u32 key) const {
    return m_pKeyboardState[key];
}

float Platform::GetMouseX() const {
    return m_mouseX;
}

float Platform::GetMouseY() const {
    return m_mouseY;
}

