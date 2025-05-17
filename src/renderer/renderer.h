#pragma once

#include "../pch.h"

constexpr float FOV_DEG = 80.0f;
constexpr float CAM_NEAR = 0.01f;
constexpr float CAM_FAR = 1000.0f;

class Renderer {
private:
    Renderer() {}
public:
    static Renderer& GetInstance();
    Renderer(const Renderer&) = delete;
    void operator=(const Renderer&) = delete;

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };
    using Index = u32;

    struct TextureData {
        void* pPixels = nullptr;
        u32 width     = 0;
        u32 height    = 0;
    };

    enum TexIdx {
        TexIdx_Albedo = 0,
        TexIdx_Normal,
        TexIdx_ARM,
        TextureCount
    };

    struct MeshCreateInfo {
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
        glm::vec2 padding;
    };

    void Initialize(SDL_Window* pWindow, u32 width, u32 height);
    ~Renderer();
    void RenderScene();
    void SetViewMatrix(const glm::mat4& viewMat);
    bool CreateMesh(const MeshCreateInfo& createInfo, const string& meshName);
    bool DeleteMesh(const string& meshName);
    glm::mat4* GetMeshTransform(const string& meshName);
    bool AddLight(const PointLight& light);
private:
    struct ShaderCreateInfo {
        SDL_GPUShaderStage stage;
        string             source;
        u32 numSamplers        = 0;
        u32 numStorageTextures = 0;
        u32 numStorageBuffers  = 0;
        u32 numUniformBuffers  = 0;
    };
    class Shader {
    public:
        void Initialize(const ShaderCreateInfo& createInfo);
        ~Shader();
        SDL_GPUShader* GetHandle();
    private:
        SDL_GPUShader* m_pHandle;
    };

    struct GfxPipelineCreateInfo {
        ShaderCreateInfo vertShaderCreateInfo;
        ShaderCreateInfo fragShaderCreateInfo;
    };
    class GfxPipeline {
    public:
        void Initialize(const GfxPipelineCreateInfo& createInfo);
        ~GfxPipeline();
        SDL_GPUGraphicsPipeline* GetHandle();
    private:
        SDL_GPUGraphicsPipeline* m_pHandle;
    };

    class UploadBuffer {
    public:
        void Initialize(u32 byteSize);
        ~UploadBuffer();
        SDL_GPUTransferBuffer* GetHandle() const;
        u32 GetSize() const;
        void SetData(const void* pData, u32 byteSize);
    private:
        SDL_GPUTransferBuffer* m_pHandle;
        u32 m_byteSize;
    };

    class Buffer {
    public:
        void Initialize(SDL_GPUCommandBuffer* pCmdBuf, SDL_GPUBufferUsageFlags usage, u32 byteSize);
        ~Buffer();
        SDL_GPUBuffer* GetHandle() const;
        void Upload(SDL_GPUCommandBuffer* pCmdBuf, const UploadBuffer& uploadBuf, u32 byteSize = 0);
        void Upload(SDL_GPUCommandBuffer* pCmdBuf, const void* pData, u32 byteSize);
    private:
        SDL_GPUBuffer* m_pHandle;
    };

    struct SamplerCreateInfo {
        SDL_GPUSamplerAddressMode addressMode = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        SDL_GPUFilter             minFilter   = SDL_GPU_FILTER_LINEAR;
        SDL_GPUFilter             magFilter   = SDL_GPU_FILTER_LINEAR;
    };
    class Sampler {
    public:
        void Initialize(const SamplerCreateInfo& createInfo);
        ~Sampler();
        SDL_GPUSampler* GetHandle() const;
    private:
        SDL_GPUSampler* m_pHandle;
    };

    struct TextureCreateInfo {
        TextureData              data;
        SDL_GPUTextureType       type        = SDL_GPU_TEXTURETYPE_2D;
        SDL_GPUTextureFormat     format      = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        SDL_GPUTextureUsageFlags usage       = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        u32                      layerNum    = 1;
        u32                      mipLevelNum = 1;
    };
    class Texture {
    public:
        void Initialize(const TextureCreateInfo& createInfo);
        ~Texture();
        SDL_GPUTexture* GetHandle() const;
        void Upload(SDL_GPUCommandBuffer* pCmdBuf, const UploadBuffer& uploadBuf, const TextureData& data);
        void Upload(SDL_GPUCommandBuffer* pCmdBuf, const TextureData& data);
    private:
        SDL_GPUTexture* m_pHandle;
    };
    
    struct Mesh {
        glm::mat4                    transform;
        Buffer                       vertexBuffer;
        Buffer                       indexBuffer;
        u32                          indicesNum;
        array<Texture, TextureCount> textures;
    };

    //inline static SDL_Window*    s_pWindow;
    //inline static SDL_GPUDevice* s_pDevice;

    glm::mat4 m_view = glm::mat4(1);
    glm::mat4 m_proj = glm::mat4(1);
    umap<string, Mesh> m_meshes;
    vector<PointLight> m_pointLights;

    GfxPipeline m_pipeline;
    Sampler     m_sampler;
    Texture     m_depthTexture;
    Buffer      m_pointLightsBuffer;

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

    // NOTE: check if std::function<> hurts performance in the future
    using CommandBufferFunction = std::function<void(SDL_GPUCommandBuffer*)>;
    void ImmediateCmdBuf(CommandBufferFunction function);

    static SDL_GPUDevice*& GetDevice();
    static SDL_Window*& GetWindow();

    void DrawMesh(const Mesh& mesh, SDL_GPURenderPass* pRenderPass, SDL_GPUCommandBuffer* pCmdBuf);
};


