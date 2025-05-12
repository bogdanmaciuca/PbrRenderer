#include "pch.h"
#include "platform.h"
#include "renderer/renderer.h"

int main() {
    Platform platform("PBR Renderer", 1024, 768);
    Renderer renderer(platform.GetSDLWindow());

    while (!platform.ShouldClose()) {
        platform.HandleEvents();
    }
}

