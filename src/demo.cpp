/*
 * TODO:
 * - initialize mesh in Renderer::CreateMesh() [done|not tested]
 * - assimp PBR loading [done|not tested]
 * - forward rendering
 * - RAII abstractions for GPU objects
 */
#include "pch.h"
#include "platform.h"
#include "renderer/renderer.h"

constexpr float TARGET_FPS = 60.0f;

Renderer::MeshData LoadMesh(const string& path) {
    Renderer::MeshData meshData;

    Assimp::Importer importer;
    const aiScene *pScene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_OptimizeGraph |
        aiProcess_GenNormals
    );

    if (pScene == nullptr || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode)
        FatalError("Could not load mesh: " + path);

    SDL_assert(pScene->mNumMeshes == 1);
    SDL_assert(pScene->mMeshes[0]->mTextureCoords[0] != nullptr);

    meshData.vertices.resize(pScene->mMeshes[0]->mNumVertices);
    meshData.indices.resize(pScene->mMeshes[0]->mNumFaces * 3);

    for (i32 i = 0; i < pScene->mMeshes[0]->mNumVertices; i++) {
        meshData.vertices[i].pos = glm::vec3(
            pScene->mMeshes[0]->mVertices[i].x,
            pScene->mMeshes[0]->mVertices[i].y,
            pScene->mMeshes[0]->mVertices[i].z
        );
        meshData.vertices[i].normal = glm::vec3(
            pScene->mMeshes[0]->mNormals[i].x,
            pScene->mMeshes[0]->mNormals[i].y,
            pScene->mMeshes[0]->mNormals[i].z
        );
        meshData.vertices[i].texCoord = glm::vec3(
            pScene->mMeshes[0]->mTextureCoords[0][i].x,
            pScene->mMeshes[0]->mTextureCoords[0][i].y,
            pScene->mMeshes[0]->mTextureCoords[0][i].z
        );
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
        meshData.texturesData[i].width   = width;
        meshData.texturesData[i].height  = height;
        meshData.texturesData[i].pPixels = pPixels;
    }

    return meshData;
}

int main() {
    LoadMesh("res/axe/wooden_axe_03_1k.gltf");
    exit(0);

    Platform platform("PBR Renderer", 1024, 768);
    Renderer renderer(platform.GetSDLWindow());

    Timer frameTimer;
    float renderAccumulator = 0.0f;

    Timer test;
    while (!platform.ShouldClose()) {
        float deltaTime = frameTimer.GetTime();
        frameTimer.Reset();

        renderAccumulator += deltaTime;
        
        if (renderAccumulator > 1000.0f / TARGET_FPS) {
            renderAccumulator = 0.0f;
            // Render
        }
        else {
            platform.HandleEvents();
        }
    }
}

