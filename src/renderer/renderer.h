#pragma once

#include "../pch.h"

class Renderer {
public:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };
    using Index = u32;
    struct TextureData {
        void* pPixels;
        u32 width, height;
    };

    struct MeshData {
        enum TextureIndex {
            Albedo = 0,
            Normal,
            ARM,
            TextureCount
        };
        vector<Vertex> vertices;
        vector<Index> indices;
        array<TextureData, 3> texturesData;
    };

    struct Mesh {
        SDL_GPUBuffer* pVertexBuffer;
        SDL_GPUBuffer* pIndexBuffer;
        array<SDL_GPUTexture*, 3> textures;

        glm::vec3 pos;
        glm::vec3 rotation;
    };

    Renderer(SDL_Window* pWindow);
    ~Renderer();
    void RenderScene();
    bool CreateMesh(const MeshData& meshData, const string& meshName);
    bool DeleteMesh(const string& meshName);
    Mesh* GetMesh(const string& meshName);
private:
    umap<string, Mesh> m_meshes;

    SDL_Window* m_pWindow;
    SDL_GPUDevice* m_pDevice;
    SDL_GPUSampler* m_pSampler;

    SDL_GPUTransferBuffer* CreateUploadBuffer(u32 size);
    SDL_GPUBuffer* CreateBuffer(SDL_GPUCommandBuffer* pCmdBuf, const void* pData, u32 dataSize, u32 usage);
    void UploadToBuffer(SDL_GPUCommandBuffer* pCmdBuf, SDL_GPUBuffer* pBuffer, const void* data, u32 dataSize);
    SDL_GPUSampler* CreateSampler();
    SDL_GPUTexture* CreateTexture(SDL_GPUCommandBuffer* pCmdBuf, const TextureData& texData);
    void UploadToTexture(SDL_GPUCommandBuffer* pCmdBuf, SDL_GPUTexture* pTexture, const TextureData& texData);
};


