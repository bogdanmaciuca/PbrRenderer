#pragma once

#include "../pch.h"

constexpr float FOV_DEG = 80.0f;
constexpr float CAM_NEAR = 0.01f;
constexpr float CAM_FAR = 1000.0f;

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

    enum TexIdx {
        TexIdx_Albedo = 0,
        TexIdx_Normal,
        TexIdx_ARM,
        TextureCount
    };

    struct MeshData {
        vector<Vertex> vertices;
        vector<Index> indices;
        array<TextureData, TextureCount> texturesData;
    };

    struct PointLight {
        glm::vec3 pos;
        glm::vec3 color;
    };

    struct Mesh {
        SDL_GPUBuffer* pVertexBuffer;
        SDL_GPUBuffer* pIndexBuffer;
        u32 indicesNum;
        array<SDL_GPUTexture*, TextureCount> textures;

        glm::vec3 pos;
        glm::vec3 rotation;
    };

    Renderer(SDL_Window* pWindow, u32 width, u32 height);
    ~Renderer();
    void RenderScene();
    void SetViewMatrix(const glm::mat4& viewMat);
    bool CreateMesh(const MeshData& meshData, const string& meshName);
    bool DeleteMesh(const string& meshName);
    Mesh* GetMesh(const string& meshName);
    bool AddLight(const PointLight& light, const string& lightName);
    PointLight* GetLight(const string& lightName);
private:
    struct ShaderInfo {
        u32 numSamplers        = 0;
        u32 numStorageTextures = 0;
        u32 numStorageBuffers  = 0;
        u32 numUniformBuffers  = 0;
    };
    struct ModelViewProj {
        glm::mat4 model = glm::mat4(1);
        glm::mat4 view  = glm::mat4(1);
        glm::mat4 proj  = glm::mat4(1);
    };
    struct Camera {
        glm::vec3 m_pos = glm::vec3(0, 0, 0);
        float m_yaw = 0, m_pitch = 0;
    };

    ModelViewProj m_MVP;
    umap<string, Mesh> m_meshes;
    umap<string, PointLight> m_pointLights;

    SDL_Window* m_pWindow;
    SDL_GPUDevice* m_pDevice;
    SDL_GPUGraphicsPipeline* m_pPipeline;
    SDL_GPUSampler* m_pSampler;

    static constexpr std::array<SDL_GPUVertexAttribute, 3> s_vertexAttribs = {
        SDL_GPUVertexAttribute{
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(Vertex, pos)
        },
        SDL_GPUVertexAttribute{
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(Vertex, normal)
        },
        SDL_GPUVertexAttribute{
            .location = 2,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            .offset = offsetof(Vertex, texCoord)
        }
    };

    SDL_GPUShader* CreateShader(const string& source, SDL_GPUShaderStage shaderStage, const ShaderInfo& shaderInfo);
    SDL_GPUGraphicsPipeline* CreateGraphicsPipeline(const string& vertSrc, const ShaderInfo& vertInfo, const string& fragSrc, const ShaderInfo& fragInfo);
    SDL_GPUTransferBuffer* CreateUploadBuffer(u32 size);
    SDL_GPUBuffer* CreateBuffer(SDL_GPUCommandBuffer* pCmdBuf, const void* pData, u32 dataSize, u32 usage);
    void UploadToBuffer(SDL_GPUCommandBuffer* pCmdBuf, SDL_GPUBuffer* pBuffer, const void* data, u32 dataSize);
    SDL_GPUSampler* CreateSampler();
    SDL_GPUTexture* CreateTexture(SDL_GPUCommandBuffer* pCmdBuf, const TextureData& texData);
    void UploadToTexture(SDL_GPUCommandBuffer* pCmdBuf, SDL_GPUTexture* pTexture, const TextureData& texData);

    void DrawMesh(const Mesh& mesh, SDL_GPURenderPass* pRenderPass);
};


