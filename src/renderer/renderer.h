#pragma once

#include "../pch.h"

constexpr float FOV_DEG  = 80.0f;
constexpr float CAM_NEAR = 0.01f;
constexpr float CAM_FAR  = 1000.0f;
constexpr u32   MAX_POINT_LIGHT_NUM = 1024;

class Renderer {
private:
    Renderer() {}
public:
    static Renderer& GetInstance();
    Renderer(const Renderer&) = delete;
    void operator=(const Renderer&) = delete;

    using Vec2 = glm::vec2;
    using Vec3 = glm::vec3;
    using Vec4 = glm::vec4;
    using Mat4 = glm::mat4;

    struct Vertex {
        Vec3 pos;
        Vec3 normal;
        Vec3 tangent;
        Vec2 texCoord;
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

    struct PointLight {
        Vec3  pos;
        float radius;
        Vec3  color = Vec3(1);
        u32   padding0;
    };

    void Initialize(SDL_Window* pWindow, u32 width, u32 height);
    ~Renderer();
    SDL_GPUDevice* GetDevicePtr();
    SDL_GPUTextureFormat GetColorTargetFormat();
    void HandleResize(u32 newWidth, u32 newHeight);
    void RenderFrame();
    void SetViewMatrix(const glm::mat4& viewMat);
    bool CreateMesh(const MeshCreateInfo& createInfo, const string& meshName);
    bool DeleteMesh(const string& meshName);
    glm::mat4* GetMeshTransform(const string& meshName);
    void SetCameraPos(const Vec3& camPos);
    void PushPointLight(const PointLight& pointLight); // NOTE: point lights are reset on every new frame
    void ClearPointLights();
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
        void Release();
        SDL_GPUTexture* GetHandle() const;
        void Upload(SDL_GPUCommandBuffer* pCmdBuf, const UploadBuffer& uploadBuf, const TextureData& data);
        void Upload(SDL_GPUCommandBuffer* pCmdBuf, const TextureData& data);
    private:
        SDL_GPUTexture* m_pHandle = nullptr;
    };
    
    struct Mesh {
        glm::mat4                    transform = Mat4(1);
        Buffer                       vertexBuffer;
        Buffer                       indexBuffer;
        u32                          indicesNum;
        array<Texture, TextureCount> textures;
    };

    struct FragmentShaderFrameData {
        Vec3       camPos;
        u32        padding0;
        Vec3       dirLight;
        u32        pointLightNum;
        PointLight pointLights[MAX_POINT_LIGHT_NUM];
    };
    FragmentShaderFrameData m_fragmentShaderFrameData;
    Buffer m_fragmentShaderFrameDataBuffer;

    glm::mat4 m_proj = glm::mat4(1);
    glm::mat4 m_view;
    umap<string, Mesh> m_meshes;

    GfxPipeline m_pipeline;
    Sampler     m_sampler;
    Texture     m_depthTexture;

    static constexpr std::array<SDL_GPUVertexAttribute, 4> s_vertexAttribs = {
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
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(Vertex, tangent)
        },
        SDL_GPUVertexAttribute{
            .location = 3,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            .offset = offsetof(Vertex, texCoord)
        }
    };

    static SDL_GPUDevice*& GetDevice();
    static SDL_Window*& GetWindow();

    // NOTE: check if std::function<> hurts performance in the future
    using CommandBufferFunction = std::function<void(SDL_GPUCommandBuffer*)>;
    void ImmediateCmdBuf(CommandBufferFunction function);
    void UpdateProjection(u32 width, u32 height);

    void UpdateFragmentShaderFrameData(SDL_GPUCommandBuffer* pCmdBuf);
    void PushFragmentShaderFrameData(SDL_GPURenderPass* pRenderPass);

    void DrawMesh(const Mesh& mesh, SDL_GPURenderPass* pRenderPass, SDL_GPUCommandBuffer* pCmdBuf);
};


