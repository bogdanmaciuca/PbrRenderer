/*
 * TODO:
 * - forward rendering
 *   - point lights (storage buffers)
 *   - normal maps, tangent space
 *   - PBR equations
 * - storing rotation in the Mesh class
 */
#include "pch.h"
#include "platform.h"
#include "renderer/renderer.h"

constexpr u32 WND_W = 1024;
constexpr u32 WND_H = 768;
constexpr float TARGET_FPS = 60.0f;

class Camera {
public:
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(m_pos, m_pos + m_front, s_up);
    }
    void MoveForward(float deltaTime) {
        m_pos += m_front * s_speed * deltaTime;
    }
    void MoveBackword(float deltaTime) {
        m_pos -= m_front * s_speed * deltaTime;
    }
    void MoveLeft(float deltaTime) {
        m_pos -= glm::cross(m_front, s_up) * s_speed * deltaTime;
    }
    void MoveRight(float deltaTime) {
        m_pos += glm::cross(m_front, s_up) * s_speed * deltaTime;
    }
    void ProcessMouse(float deltaX, float deltaY) {
        m_yaw += deltaX * s_sensitivity;
        m_pitch += deltaY * s_sensitivity;
        m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);

        m_front.x = glm::cos(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
        m_front.y = glm::sin(glm::radians(m_pitch));
        m_front.z = glm::sin(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
        m_front = glm::normalize(m_front);
    }
private:
    glm::vec3 m_pos = glm::vec3(0.0f);
    float m_yaw = 0.0f, m_pitch = 0.0f; // Degrees
    glm::vec3 m_front = glm::vec3(0.0f);
    static constexpr glm::vec3 s_up = glm::vec3(0, 1, 0);
    static constexpr float s_speed = 0.002f;
    static constexpr float s_sensitivity = 0.35f;
};

Renderer::MeshCreateInfo LoadMesh(const string& path) {
    Renderer::MeshCreateInfo meshCreateInfo;

    Assimp::Importer importer;
    const aiScene *pScene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_OptimizeGraph |
        aiProcess_GenNormals |
        aiProcess_FixInfacingNormals
    );

    if (pScene == nullptr || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode)
        FatalError("Could not load mesh: " + path);

    SDL_assert(pScene->mNumMeshes == 1);
    SDL_assert(pScene->mMeshes[0]->mTextureCoords[0] != nullptr);

    meshCreateInfo.vertices.resize(pScene->mMeshes[0]->mNumVertices);
    meshCreateInfo.indices.resize(pScene->mMeshes[0]->mNumFaces * 3);

    for (i32 i = 0; i < pScene->mMeshes[0]->mNumVertices; i++) {
        meshCreateInfo.vertices[i].pos = glm::vec3(
            pScene->mMeshes[0]->mVertices[i].x,
            pScene->mMeshes[0]->mVertices[i].y,
            pScene->mMeshes[0]->mVertices[i].z
        );
        meshCreateInfo.vertices[i].normal = glm::vec3(
            pScene->mMeshes[0]->mNormals[i].x,
            pScene->mMeshes[0]->mNormals[i].y,
            pScene->mMeshes[0]->mNormals[i].z
        );
        meshCreateInfo.vertices[i].texCoord = glm::vec3(
            pScene->mMeshes[0]->mTextureCoords[0][i].x,
            pScene->mMeshes[0]->mTextureCoords[0][i].y,
            pScene->mMeshes[0]->mTextureCoords[0][i].z
        );
    }

    for (i32 i = 0; i < pScene->mMeshes[0]->mNumFaces; i++) {
        SDL_assert(pScene->mMeshes[0]->mFaces[i].mNumIndices == 3);
        meshCreateInfo.indices[i * 3 + 0] = pScene->mMeshes[0]->mFaces[i].mIndices[0];
        meshCreateInfo.indices[i * 3 + 1] = pScene->mMeshes[0]->mFaces[i].mIndices[1];
        meshCreateInfo.indices[i * 3 + 2] = pScene->mMeshes[0]->mFaces[i].mIndices[2];
    }

    SDL_assert(pScene->mNumMaterials >= 1);
    SDL_Log("Number of materials: %d", pScene->mNumMaterials);
    SDL_Log("Diffuse texture count: %d", pScene->mMaterials[0]->GetTextureCount(aiTextureType_DIFFUSE));
    SDL_Log("Normals texture count: %d", pScene->mMaterials[0]->GetTextureCount(aiTextureType_NORMALS));
    SDL_Log("ARM texture count: %d", pScene->mMaterials[0]->GetTextureCount(aiTextureType_UNKNOWN));
    SDL_Log("");

    aiString aiDiffusePath;
    pScene->mMaterials[0]->GetTexture(aiTextureType_DIFFUSE, 0, &aiDiffusePath);
    SDL_Log("Diffuse path: %s", aiDiffusePath.C_Str());
    aiString aiNormalPath;
    pScene->mMaterials[0]->GetTexture(aiTextureType_NORMALS, 0, &aiNormalPath);
    SDL_Log("Normal path: %s", aiNormalPath.C_Str());
    aiString aiArmPath;
    pScene->mMaterials[0]->GetTexture(aiTextureType_UNKNOWN, 0, &aiArmPath);
    SDL_Log("ARM path: %s", aiArmPath.C_Str());

    // NOTE: the order must match the order of the MeshData texture type enum
    const array<string, 3> texturePaths = {
        std::filesystem::path(path).parent_path().string() + "/" + aiDiffusePath.C_Str(),
        std::filesystem::path(path).parent_path().string() + "/" + aiNormalPath.C_Str(),
        std::filesystem::path(path).parent_path().string() + "/" + aiArmPath.C_Str(),
    };

    for (i32 i = 0; i < texturePaths.size(); i++) {
        int width, height, channelNum;
        u8* pPixels = stbi_load(texturePaths[i].c_str(), &width, &height, &channelNum, 4);
        SDL_assert(pPixels != nullptr);
        meshCreateInfo.texturesData[i].width   = width;
        meshCreateInfo.texturesData[i].height  = height;
        meshCreateInfo.texturesData[i].pPixels = pPixels;
    }

    return meshCreateInfo;
}

int main() {
    Platform platform("PBR Renderer", WND_W, WND_H);
    Renderer& renderer = Renderer::GetInstance();
    renderer.Initialize(platform.GetSDLWindow(), WND_W, WND_H);

    Camera camera;

    Renderer::MeshCreateInfo meshCreateInfo = LoadMesh("res/axe/wooden_axe_03_1k.gltf");
    renderer.CreateMesh(meshCreateInfo, "axe");
    renderer.AddLight(Renderer::PointLight{ .pos = glm::vec3(1, 1, 0.5) });

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
            renderer.RenderScene();
        }
        else {
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
        }
    }
}

