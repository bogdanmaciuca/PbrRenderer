#include "pch.h"

void FatalError(const string& message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal error", message.c_str(), nullptr);
    SDL_Quit();
}

