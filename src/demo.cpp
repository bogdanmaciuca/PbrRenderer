/*
 * TODO:
 * - deferred rendering
 *   - rendering to texture
 *   - applying post-effects
 * - storing rotation in the Mesh class
 * - add Release() to Renderer objects
 */
#include "pch.h"
#include "platform.h"
#include "renderer/renderer.h"
#include "camera.h"
#include "model.h"

constexpr u32 WND_W = 1024;
constexpr u32 WND_H = 768;
constexpr float TARGET_FPS = 60.0f;

int main() {
    Platform platform("PBR Renderer", WND_W, WND_H);

    Renderer& renderer = Renderer::GetInstance();
    renderer.Initialize(platform.GetSDLWindow(), WND_W, WND_H);

    platform.SetQuitCallback([](const auto& evt) { exit(0); });
    platform.SetResizeCallback([&](const SDL_Event& evt) {
        renderer.HandleResize(evt.window.data1, evt.window.data2);
    });

    Camera camera;

    Renderer::MeshCreateInfo meshCreateInfo = LoadMesh("C:/Users/Bogdan/Documents/C_Projects/PbrRenderer/res/axe/wooden_axe_03_1k.gltf");
    renderer.CreateMesh(meshCreateInfo, "axe");

    float angle = 0.0f;

    Timer frameTimer;
    float renderAccumulator = 0.0f;

    float lastMouseX = 0.0f, lastMouseY = 0.0f;

    Timer test;
    while (!platform.ShouldClose()) {
        float deltaTime = frameTimer.GetTime();
        frameTimer.Reset();

        renderAccumulator += deltaTime;
        
        if (renderAccumulator > 1000.0f / TARGET_FPS) {
            renderAccumulator = 0.0f;
            // Actual rendering code
            renderer.ClearPointLights();
            Renderer::PointLight light = {
                .pos = Renderer::Vec3(0, sin(angle), cos(angle))
            };
            renderer.PushPointLight(light);
            renderer.RenderScene();
        }
        else {
            angle += deltaTime * 0.001f;

            platform.HandleEvents();
            // Mouse
            float mouseX = platform.GetMouseX();
            float mouseY = platform.GetMouseY();
            float deltaX = mouseX - lastMouseX;
            float deltaY = mouseY - lastMouseY;
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            camera.ProcessMouse(deltaX, -deltaY);

            // Keyboard
            if (platform.KeyIsDown(SDL_SCANCODE_W))
                camera.MoveForward(deltaTime);
            else if (platform.KeyIsDown(SDL_SCANCODE_S))
                camera.MoveBackword(deltaTime);
            if (platform.KeyIsDown(SDL_SCANCODE_A))
                camera.MoveLeft(deltaTime);
            else if (platform.KeyIsDown(SDL_SCANCODE_D))
                camera.MoveRight(deltaTime);

            renderer.SetViewMatrix(camera.GetViewMatrix());

            renderer.SetCameraPos(camera.GetPos());
        }
    }
}

