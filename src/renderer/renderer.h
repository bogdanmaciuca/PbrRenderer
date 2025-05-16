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

    enum {
        POINT_LIGHTS_MAX_NUM = 16
    };

    struct PointLight {
        glm::vec3 pos;
        glm::vec3 color = glm::vec3(1.0f);
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
    // TODO: use the lightName parameter
    bool AddLight(const PointLight& light, const string& lightName);
    PointLight* GetLight(const string& lightName);
private:
    struct ShaderInfo {
        u32 numSamplers        = 0;
        u32 numStorageTextures = 0;
        u32 numStorageBuffers  = 0;
        u32 numUniformBuffers  = 0;
    };
    struct Camera {
        glm::vec3 m_pos = glm::vec3(0, 0, 0);
        float m_yaw     = 0;
        float m_pitch   = 0;
    };

    glm::mat4 m_view = glm::mat4(1);
    glm::mat4 m_proj = glm::mat4(1);
    umap<string, Mesh> m_meshes;
    vector<PointLight> m_pointLights;

    SDL_Window*              m_pWindow;
    SDL_GPUDevice*           m_pDevice;
    SDL_GPUGraphicsPipeline* m_pPipeline;
    SDL_GPUSampler*          m_pSampler;
    SDL_GPUTexture*          m_pDepthTexture;
    SDL_GPUBuffer*           m_pPointLightsBuffer;

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
    SDL_GPUBuffer* CreateBuffer(SDL_GPUCommandBuffer* pCmdBuf, const void* pData, u32 dataSize, SDL_GPUBufferUsageFlags usage);
    void UploadToBuffer(SDL_GPUCommandBuffer* pCmdBuf, SDL_GPUBuffer* pBuffer, const void* data, u32 dataSize);
    SDL_GPUSampler* CreateSampler();
    SDL_GPUTexture* CreateTexture(SDL_GPUCommandBuffer* pCmdBuf, const TextureData& texData, SDL_GPUTextureType type = SDL_GPU_TEXTURETYPE_2D, SDL_GPUTextureUsageFlags usage = SDL_GPU_TEXTUREUSAGE_SAMPLER);
    void UploadToTexture(SDL_GPUCommandBuffer* pCmdBuf, SDL_GPUTexture* pTexture, const TextureData& texData);

    void DrawMesh(const Mesh& mesh, SDL_GPURenderPass* pRenderPass, SDL_GPUCommandBuffer* pCmdBuf);
};


