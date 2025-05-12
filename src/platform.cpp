#include "platform.h"

#include "pch.h"

Platform::Platform(const string& wndName, i32 wndWidth, i32 wndHeight) {
    if (SDL_Init(SDL_INIT_VIDEO) == false)
        FatalError("Could not initialize SDL");

    m_pWindow = SDL_CreateWindow(wndName.c_str(), wndWidth, wndHeight, 0);
    if (m_pWindow == nullptr)
        FatalError("Could not create window");
}

Platform::~Platform() {
    SDL_DestroyWindow(m_pWindow);
}

bool Platform::ShouldClose() {
    return m_shouldClose;
}

SDL_Window* Platform::GetSDLWindow() {
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
}
