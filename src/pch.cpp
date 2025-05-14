#include "pch.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

void Error(const string& message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal error", message.c_str(), nullptr);
}

void FatalError(const string& message) {
    Error(message);
    SDL_Quit();
}

