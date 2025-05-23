#include "platform.h"

#include "pch.h"

Platform::Platform(const string& wndName, i32 wndWidth, i32 wndHeight)
    : m_pKeyboardState(SDL_GetKeyboardState(nullptr))
{
    if (SDL_Init(SDL_INIT_VIDEO) == false)
        FatalError("Could not initialize SDL");

    SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    m_pWindow = SDL_CreateWindow(wndName.c_str(), wndWidth, wndHeight, windowFlags);
    if (m_pWindow == nullptr)
        FatalError("Could not create window");
}

Platform::~Platform() {
    SDL_DestroyWindow(m_pWindow);
}

void Platform::SetQuitCallback(const EventCallback& callback) {
    m_quitCallback = callback;
}

void Platform::SetResizeCallback(const EventCallback& callback) {
    m_resizeCallback = callback;
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
        ImGui_ImplSDL3_ProcessEvent(&evt);
        switch (evt.type) {
            case SDL_EVENT_QUIT:
                if (m_quitCallback) m_quitCallback(evt);
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                SDL_GetWindowSurface(m_pWindow);
                SDL_UpdateWindowSurface(m_pWindow);
                if (m_resizeCallback) m_resizeCallback(evt);
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

