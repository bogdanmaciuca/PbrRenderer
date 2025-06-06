#include "renderer.h"

#include "../pch.h"

Renderer& Renderer::GetInstance() {
    static Renderer instance;
    return instance;
}

void Renderer::Initialize(SDL_Window* pWindow, u32 width, u32 height) {
    GetWindow() = pWindow;

    bool debugMode = true;
    GetDevice() = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, debugMode, nullptr);
    if (GetDevice() == nullptr)
        FatalError("Could not create GPU device");

    if (SDL_ClaimWindowForGPUDevice(GetDevice(), GetWindow()) == false)
        FatalError("Could not claim window for GPU device");

    SamplerCreateInfo samplerCreateInfo;
    m_sampler.Initialize(samplerCreateInfo);

    const GfxPipelineCreateInfo pipelineCreateInfo = {
        .vertShaderCreateInfo = {
            .stage  = SDL_GPU_SHADERSTAGE_VERTEX,
            .source = ReadFile("C:/Users/Bogdan/Documents/C_Projects/PbrRenderer/shaders_compiled/basic.vert.spv"),
            .numUniformBuffers = 3
        },
        .fragShaderCreateInfo = {
            .stage  = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .source = ReadFile("C:/Users/Bogdan/Documents/C_Projects/PbrRenderer/shaders_compiled/basic.frag.spv"),
            .numSamplers       = 3,
            .numStorageBuffers = 1
        }
    };
    m_pipeline.Initialize(pipelineCreateInfo);

    UpdateProjection(width, height);

    ImmediateCmdBuf([&](SDL_GPUCommandBuffer* pCmdBuf) {
        // Depth texture
        TextureCreateInfo depthTextureCreateInfo;
        depthTextureCreateInfo.usage  = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        depthTextureCreateInfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
        depthTextureCreateInfo.data.width  = width;
        depthTextureCreateInfo.data.height = height;
        m_depthTexture.Initialize(depthTextureCreateInfo);

        // Storage buffer for vertex shader frame data
        m_fragmentShaderFrameDataBuffer.Initialize(
            pCmdBuf,
            SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            sizeof(FragmentShaderFrameData)
        );
    });
}

Renderer::~Renderer() {
    SDL_ReleaseWindowFromGPUDevice(GetDevice(), GetWindow());
    SDL_DestroyGPUDevice(GetDevice());
}

SDL_GPUDevice* Renderer::GetDevicePtr() {
    return GetDevice();
}
SDL_GPUTextureFormat Renderer::GetColorTargetFormat() {
    return SDL_GetGPUSwapchainTextureFormat(GetDevice(), GetWindow());
}

void Renderer::HandleResize(u32 newWidth, u32 newHeight) {
    UpdateProjection(newWidth, newHeight);

    ImmediateCmdBuf([&](SDL_GPUCommandBuffer* pCmdBuf) {
        TextureCreateInfo depthTextureCreateInfo;
        depthTextureCreateInfo.usage  = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        depthTextureCreateInfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
        depthTextureCreateInfo.data.width  = newWidth;
        depthTextureCreateInfo.data.height = newHeight;

        m_depthTexture.Release();
        m_depthTexture.Initialize(depthTextureCreateInfo);
    });
}

void Renderer::RenderFrame() {
    SDL_GPUCommandBuffer* pCmdBuf = SDL_AcquireGPUCommandBuffer(GetDevice());

    // ImGui
    ImDrawData* pDrawData = ImGui::GetDrawData();
    if (pDrawData != nullptr)
        ImGui_ImplSDLGPU3_PrepareDrawData(pDrawData, pCmdBuf);

    // Copy pass for updating fragment shader frame data
    UpdateFragmentShaderFrameData(pCmdBuf);

    // Get swapchain texture
    SDL_GPUTexture* pSwapchainTexture;
    bool result = SDL_WaitAndAcquireGPUSwapchainTexture(pCmdBuf, GetWindow(), &pSwapchainTexture, nullptr, nullptr);
    if (result == false)
        FatalError("Could not acquire GPUSwapchain Texture");
    // Return early if window is minimized
    if (pSwapchainTexture == nullptr) {
        SDL_SubmitGPUCommandBuffer(pCmdBuf);
        return;
    }
    
    // Begin render pass
    SDL_GPUColorTargetInfo colorTargetInfo = {
        .texture = pSwapchainTexture,
        .clear_color = SDL_FColor{ .r = 0.1f, .g = 0.15f, .b = 0.2f, .a = 1.0f },
        .load_op     = SDL_GPU_LOADOP_CLEAR,
        .store_op    = SDL_GPU_STOREOP_STORE
    };
    SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo = { 0 };
    depthStencilTargetInfo.texture          = m_depthTexture.GetHandle();
    depthStencilTargetInfo.cycle            = true;
    depthStencilTargetInfo.clear_depth      = 1;
    depthStencilTargetInfo.clear_stencil    = 0;
    depthStencilTargetInfo.load_op          = SDL_GPU_LOADOP_CLEAR;
    depthStencilTargetInfo.store_op         = SDL_GPU_STOREOP_STORE;
    depthStencilTargetInfo.stencil_load_op  = SDL_GPU_LOADOP_CLEAR;
    depthStencilTargetInfo.stencil_store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass* pRenderPass = SDL_BeginGPURenderPass(pCmdBuf, &colorTargetInfo, 1, &depthStencilTargetInfo);

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(pRenderPass, m_pipeline.GetHandle());

    // Vertex shader frame data
    constexpr u32 projSlotIdx = 0;
    constexpr u32 viewSlotIdx = 2;
    SDL_PushGPUVertexUniformData(pCmdBuf, viewSlotIdx, &m_view, sizeof(Mat4));
    // Bind other stuff (temporary; TODO: only do these when the information is updated)
    SDL_PushGPUVertexUniformData(pCmdBuf, projSlotIdx, &m_proj, sizeof(Mat4));

    // Fragment shader frame data
    PushFragmentShaderFrameData(pRenderPass);

    // Draw meshes
    for (auto& it : m_meshes)
        DrawMesh(it.second, pRenderPass, pCmdBuf);

    if (pDrawData != nullptr)
        ImGui_ImplSDLGPU3_RenderDrawData(pDrawData, pCmdBuf, pRenderPass);

    SDL_EndGPURenderPass(pRenderPass);

    SDL_SubmitGPUCommandBuffer(pCmdBuf);
}

void Renderer::SetViewMatrix(const Mat4& viewMat) {
    m_view = viewMat;
}

bool Renderer::CreateMesh(const MeshCreateInfo& createInfo, const string& meshName) {
    if (m_meshes.contains(meshName))
        return false;

    Mesh& mesh = m_meshes[meshName] = Mesh();

    ImmediateCmdBuf([&](SDL_GPUCommandBuffer* pCmdBuf) {
        // Vertex buffer
        mesh.vertexBuffer.Initialize(
            pCmdBuf,
            SDL_GPU_BUFFERUSAGE_VERTEX,
            createInfo.vertices.size() * sizeof(Vertex)
        );
        mesh.vertexBuffer.Upload(
            pCmdBuf,
            createInfo.vertices.data(),
            createInfo.vertices.size() * sizeof(Vertex)
        );

        // Index buffer
        mesh.indexBuffer.Initialize(
            pCmdBuf,
            SDL_GPU_BUFFERUSAGE_INDEX,
            createInfo.indices.size() * sizeof(Index)
        );
        mesh.indexBuffer.Upload(
            pCmdBuf,
            createInfo.indices.data(),
            createInfo.indices.size() * sizeof(Index)
        );
        mesh.indicesNum = createInfo.indices.size();

        // Textures
        for (i32 i = 0; i < TextureCount; i++) {
            TextureCreateInfo textureCreateInfo;
            textureCreateInfo.data = createInfo.texturesData[i];
            mesh.textures[i].Initialize(textureCreateInfo);
            mesh.textures[i].Upload(pCmdBuf, textureCreateInfo.data);
        }
    });

    return true;
}

bool Renderer::DeleteMesh(const string& meshName) {
    if (!m_meshes.contains(meshName))
        return false;
    m_meshes.erase(meshName);
    return true;
}

Renderer::Mat4* Renderer::GetMeshTransform(const string& meshName) {
    if (!m_meshes.contains(meshName))
        return nullptr;
    return &m_meshes[meshName].transform;
}

void Renderer::SetCameraPos(const Vec3& camPos) {
    m_fragmentShaderFrameData.camPos = camPos;
}

void Renderer::PushPointLight(const PointLight& pointLight) {
    SDL_assert(m_fragmentShaderFrameData.pointLightNum < MAX_POINT_LIGHT_NUM);
    m_fragmentShaderFrameData.pointLights[m_fragmentShaderFrameData.pointLightNum++] = pointLight;
}

void Renderer::ClearPointLights() {
    m_fragmentShaderFrameData.pointLightNum = 0;
}

void Renderer::Shader::Initialize(const ShaderCreateInfo& createInfo) {
    SDL_GPUShaderCreateInfo vertShaderCreateInfo = {
        .code_size            = createInfo.source.size(),
        .code                 = (u8*)createInfo.source.c_str(),
        .entrypoint           = "main",
        .format               = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage                = createInfo.stage,
        .num_samplers         = createInfo.numSamplers,
        .num_storage_textures = createInfo.numStorageTextures,
        .num_storage_buffers  = createInfo.numStorageBuffers,
        .num_uniform_buffers  = createInfo.numUniformBuffers,
    };

    m_pHandle = SDL_CreateGPUShader(GetDevice(), &vertShaderCreateInfo);

    if (m_pHandle == nullptr)
        FatalError("Could not create shader");
}

Renderer::Shader::~Shader() {
    SDL_ReleaseGPUShader(GetDevice(), m_pHandle);
}

SDL_GPUShader* Renderer::Shader::GetHandle() {
    return m_pHandle;
}

void Renderer::GfxPipeline::Initialize(const GfxPipelineCreateInfo& createInfo) {
    Shader vertShader;
    vertShader.Initialize(createInfo.vertShaderCreateInfo);
    Shader fragShader;
    fragShader.Initialize(createInfo.fragShaderCreateInfo);

    SDL_GPUColorTargetDescription colorTargetDesc = {};
    colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(GetDevice(), GetWindow());
    colorTargetDesc.blend_state.enable_blend   = true;
    colorTargetDesc.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTargetDesc.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTargetDesc.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTargetDesc.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTargetDesc.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTargetDesc.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

    SDL_GPUVertexBufferDescription vertBufferDesc = {};
    vertBufferDesc.slot = 0;
    vertBufferDesc.pitch = sizeof(Vertex);
    vertBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {};

    pipelineCreateInfo.target_info.num_color_targets         = 1;
    pipelineCreateInfo.target_info.color_target_descriptions = &colorTargetDesc;
    pipelineCreateInfo.target_info.has_depth_stencil_target  = true;
    pipelineCreateInfo.target_info.depth_stencil_format      = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

    pipelineCreateInfo.vertex_input_state.num_vertex_buffers         = 1;
    pipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions = &vertBufferDesc;
    pipelineCreateInfo.vertex_input_state.num_vertex_attributes      = s_vertexAttribs.size();
    pipelineCreateInfo.vertex_input_state.vertex_attributes          = s_vertexAttribs.data();

    pipelineCreateInfo.primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineCreateInfo.vertex_shader   = vertShader.GetHandle();
    pipelineCreateInfo.fragment_shader = fragShader.GetHandle();

    pipelineCreateInfo.rasterizer_state.cull_mode  = SDL_GPU_CULLMODE_NONE;
    pipelineCreateInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    pipelineCreateInfo.depth_stencil_state.enable_depth_test  = true;
    pipelineCreateInfo.depth_stencil_state.enable_depth_write = true;
    pipelineCreateInfo.depth_stencil_state.compare_op         = SDL_GPU_COMPAREOP_LESS;
    pipelineCreateInfo.depth_stencil_state.write_mask         = 0XFF;

    m_pHandle = SDL_CreateGPUGraphicsPipeline(GetDevice(), &pipelineCreateInfo);
    if (m_pHandle == nullptr)
        FatalError(string("Could not create graphics pipeline: ") + SDL_GetError());
}
Renderer::GfxPipeline::~GfxPipeline() {
    SDL_ReleaseGPUGraphicsPipeline(GetDevice(), m_pHandle);
}
SDL_GPUGraphicsPipeline* Renderer::GfxPipeline::GetHandle() {
    return m_pHandle;
}

void Renderer::UploadBuffer::Initialize(u32 byteSize) {
    m_byteSize = byteSize;

    SDL_GPUTransferBufferCreateInfo createInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size  = m_byteSize,
        .props = 0
    };
    m_pHandle = SDL_CreateGPUTransferBuffer(GetDevice(), &createInfo);
    if (m_pHandle == nullptr)
        FatalError("Could not create transfer buffer");
}
Renderer::UploadBuffer::~UploadBuffer() {
    SDL_ReleaseGPUTransferBuffer(GetDevice(), m_pHandle);
}
SDL_GPUTransferBuffer* Renderer::UploadBuffer::GetHandle() const {
    return m_pHandle;
}
u32 Renderer::UploadBuffer::GetSize() const {
    return m_byteSize;
}
void Renderer::UploadBuffer::SetData(const void* pData, u32 byteSize) {
    void* pMappedMemory = SDL_MapGPUTransferBuffer(GetDevice(), m_pHandle, false);
    SDL_assert(pMappedMemory != nullptr);
    SDL_memcpy(pMappedMemory, pData, byteSize);
    SDL_UnmapGPUTransferBuffer(GetDevice(), m_pHandle);
}

void Renderer::Buffer::Initialize(SDL_GPUCommandBuffer* pCmdBuf, SDL_GPUBufferUsageFlags usage, u32 byteSize) {
    SDL_GPUBufferCreateInfo bufCreateInfo = {
        .usage = usage,
        .size  = byteSize,
        .props = 0
    };

    m_pHandle = SDL_CreateGPUBuffer(GetDevice(), &bufCreateInfo);
    if (m_pHandle == nullptr)
        Error("Could not create buffer");
}
Renderer::Buffer::~Buffer() {
    SDL_ReleaseGPUBuffer(GetDevice(), m_pHandle);
}
SDL_GPUBuffer* Renderer::Buffer::GetHandle() const {
    return m_pHandle;
}
void Renderer::Buffer::Upload(SDL_GPUCommandBuffer* pCmdBuf, const UploadBuffer& uploadBuf, u32 byteSize) {
    SDL_GPUTransferBufferLocation transferBufferLocation = {
        .transfer_buffer = uploadBuf.GetHandle(),
        .offset          = 0
    };
    SDL_GPUBufferRegion bufferRegion = {
        .buffer = m_pHandle,
        .offset = 0,
        .size   = (byteSize != 0 ? byteSize : uploadBuf.GetSize())
    };

    SDL_GPUCopyPass* pCopyPass = SDL_BeginGPUCopyPass(pCmdBuf);
    SDL_UploadToGPUBuffer(pCopyPass, &transferBufferLocation, &bufferRegion, false);
    SDL_EndGPUCopyPass(pCopyPass);
}
void Renderer::Buffer::Upload(SDL_GPUCommandBuffer* pCmdBuf, const void* pData, u32 byteSize) {
    UploadBuffer uploadBuf;
    uploadBuf.Initialize(byteSize);
    uploadBuf.SetData(pData, byteSize);
    Upload(pCmdBuf, uploadBuf);
}

void Renderer::Sampler::Initialize(const SamplerCreateInfo& createInfo) {
    SDL_GPUSamplerCreateInfo samplerCreateInfo = {
        .min_filter     = createInfo.minFilter,
        .mag_filter     = createInfo.magFilter,
        .mipmap_mode    = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
        .address_mode_u = createInfo.addressMode,
        .address_mode_v = createInfo.addressMode,
        .address_mode_w = createInfo.addressMode,
    };
    m_pHandle = SDL_CreateGPUSampler(GetDevice(), &samplerCreateInfo);
    if (m_pHandle == nullptr)
        FatalError("Could not create sampler");
}
Renderer::Sampler::~Sampler() {
    SDL_ReleaseGPUSampler(GetDevice(), m_pHandle);
}
SDL_GPUSampler* Renderer::Sampler::GetHandle() const {
    return m_pHandle;
}

void Renderer::Texture::Initialize(const TextureCreateInfo& createInfo) {
    SDL_GPUTextureCreateInfo sdlCreateInfo = {
        .type                 = createInfo.type,
        .format               = createInfo.format,
        .usage                = createInfo.usage,
        .width                = createInfo.data.width,
        .height               = createInfo.data.height,
        .layer_count_or_depth = createInfo.layerNum,
        .num_levels           = createInfo.mipLevelNum
    };

    m_pHandle = SDL_CreateGPUTexture(GetDevice(), &sdlCreateInfo);
    if (m_pHandle == nullptr)
        FatalError("Could not create texture");
}
Renderer::Texture::~Texture() {
    Release();
}
void Renderer::Texture::Release() {
    if (m_pHandle != nullptr)
        SDL_ReleaseGPUTexture(GetDevice(), m_pHandle);
    m_pHandle = nullptr;
}
SDL_GPUTexture* Renderer::Texture::GetHandle() const {
    return m_pHandle;
}
void Renderer::Texture::Upload(SDL_GPUCommandBuffer* pCmdBuf, const UploadBuffer& uploadBuf, const TextureData& data) {
    SDL_GPUTextureTransferInfo transferInfo = {
        .transfer_buffer = uploadBuf.GetHandle(),
        .offset          = 0,
        .pixels_per_row  = 0,
        .rows_per_layer  = 0
    };

    SDL_GPUTextureRegion textureRegion = {
        .texture   = m_pHandle,
        .mip_level = 0,
        .layer     = 0,
        .x         = 0,
        .y         = 0,
        .w         = data.width,
        .h         = data.height,
        .d         = 1
    };

    SDL_GPUCopyPass* pCopyPass = SDL_BeginGPUCopyPass(pCmdBuf);
    SDL_UploadToGPUTexture(pCopyPass, &transferInfo, &textureRegion, false);
    SDL_EndGPUCopyPass(pCopyPass);
}
// NOTE: as of now, this assumes 32 bits per pixel
void Renderer::Texture::Upload(SDL_GPUCommandBuffer* pCmdBuf, const TextureData& data) {
    UploadBuffer uploadBuf;
    uploadBuf.Initialize(data.width * data.height * sizeof(u32));
    uploadBuf.SetData(data.pPixels, data.width * data.height * sizeof(u32));
    Upload(pCmdBuf, uploadBuf, data);
}

SDL_GPUDevice*& Renderer::GetDevice() {
    static SDL_GPUDevice* pDevice;
    return pDevice;
}
SDL_Window*& Renderer::GetWindow() {
    static SDL_Window* pWindow;
    return pWindow;
}

void Renderer::ImmediateCmdBuf(CommandBufferFunction function) {
    SDL_GPUCommandBuffer* pCmdBuf = SDL_AcquireGPUCommandBuffer(GetDevice());
    function(pCmdBuf);
    SDL_SubmitGPUCommandBuffer(pCmdBuf);
}

void Renderer::UpdateProjection(u32 width, u32 height) {
    m_proj = glm::perspective(glm::radians(FOV_DEG), (float)width / height, CAM_NEAR, CAM_FAR);
}

void Renderer::UpdateFragmentShaderFrameData(SDL_GPUCommandBuffer* pCmdBuf) {
    m_fragmentShaderFrameDataBuffer.Upload(
        pCmdBuf,
        &m_fragmentShaderFrameData,
        sizeof(FragmentShaderFrameData)
    );
}

void Renderer::PushFragmentShaderFrameData(SDL_GPURenderPass* pRenderPass) {
    constexpr u32 SHADER_FRAME_DATA_SLOT_IDX = 0; // why tf does this work when binding = 3 in shader???
    SDL_GPUBuffer* pBufferRawPtr = m_fragmentShaderFrameDataBuffer.GetHandle();
    SDL_BindGPUFragmentStorageBuffers(pRenderPass, SHADER_FRAME_DATA_SLOT_IDX, &pBufferRawPtr, 1);
}

void Renderer::DrawMesh(const Mesh& mesh, SDL_GPURenderPass* pRenderPass, SDL_GPUCommandBuffer* pCmdBuf) {
    // Binding sampler-texture pairs
    array<SDL_GPUTextureSamplerBinding, TextureCount> samplerBindings;
    for (i32 i = 0; i < TextureCount; i++) {
        SDL_GPUTextureSamplerBinding binding = {
            .texture = mesh.textures[i].GetHandle(),
            .sampler = m_sampler.GetHandle()
        };
        samplerBindings[i] = binding;
    }
    SDL_BindGPUFragmentSamplers(pRenderPass, 0, samplerBindings.data(), samplerBindings.size());

    // Binding vertex buffer
    SDL_GPUBufferBinding vertBufferBinding = {
        .buffer = mesh.vertexBuffer.GetHandle(),
        .offset = 0
    };
    SDL_BindGPUVertexBuffers(pRenderPass, 0, &vertBufferBinding, 1);

    // Binding index buffer
    SDL_GPUBufferBinding indexBufferBinding = {
        .buffer = mesh.indexBuffer.GetHandle(),
        .offset = 0
    };
    SDL_BindGPUIndexBuffer(pRenderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    // Model uniform
    constexpr u32 modelSlotIdx = 1;
    SDL_PushGPUVertexUniformData(pCmdBuf, modelSlotIdx, &mesh.transform, sizeof(Mat4));

    SDL_DrawGPUIndexedPrimitives(pRenderPass, mesh.indicesNum, 1, 0, 0, 0);
}

