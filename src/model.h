#pragma once

#include "pch.h"
#include "renderer/renderer.h"

Renderer::MeshCreateInfo LoadMesh(const string& path) {
    Renderer::MeshCreateInfo meshCreateInfo;

    Assimp::Importer importer;
    const aiScene *pScene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace |
        aiProcess_FlipUVs |
        aiProcess_OptimizeGraph
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
        meshCreateInfo.vertices[i].tangent = glm::vec3(
            pScene->mMeshes[0]->mTangents[i].x,
            pScene->mMeshes[0]->mTangents[i].y,
            pScene->mMeshes[0]->mTangents[i].z
        );
        meshCreateInfo.vertices[i].texCoord = glm::vec2(
            pScene->mMeshes[0]->mTextureCoords[0][i].x,
            pScene->mMeshes[0]->mTextureCoords[0][i].y
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


