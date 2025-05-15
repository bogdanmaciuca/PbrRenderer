#include "renderer.h"

#include "../pch.h"

Renderer::Renderer(SDL_Window* pWindow, u32 width, u32 height) : m_pWindow(pWindow) {
    bool debugMode = true;
    m_pDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, debugMode, nullptr);
    if (m_pDevice == nullptr)
        FatalError("Could not create GPU device");

    if (SDL_ClaimWindowForGPUDevice(m_pDevice, pWindow) == false)
        FatalError("Could not claim window for GPU device");

    m_pSampler = CreateSampler();

    const ShaderInfo vertInfo = {
        .numUniformBuffers  = 1
    };
    const ShaderInfo fragInfo = {
        .numSamplers        = 3,
        .numUniformBuffers  = 1
    };
    m_pPipeline = CreateGraphicsPipeline(
        ReadFile("shaders_compiled/basic.vert.spv"), vertInfo,
        ReadFile("shaders_compiled/basic.frag.spv"), fragInfo
    );

    m_MVP.proj = glm::perspective(glm::radians(FOV_DEG), (float)width / height, CAM_NEAR, CAM_FAR);
}

Renderer::~Renderer() {
    SDL_ReleaseWindowFromGPUDevice(m_pDevice, m_pWindow);
    SDL_DestroyGPUDevice(m_pDevice);
}

void Renderer::RenderScene() {
    SDL_GPUCommandBuffer* pCommandBuffer = SDL_AcquireGPUCommandBuffer(m_pDevice);

    // Get swapchain texture
    SDL_GPUTexture* pSwapchainTexture;
    bool result = SDL_WaitAndAcquireGPUSwapchainTexture(pCommandBuffer, m_pWindow, &pSwapchainTexture, nullptr, nullptr);
    if (result == false)
        FatalError("Could not acquire GPUSwapchain Texture");
    // Return early if window is minimized
    if (pSwapchainTexture == nullptr) {
        SDL_SubmitGPUCommandBuffer(pCommandBuffer);
        return;
    }

    // Begin render pass
    SDL_GPUColorTargetInfo colorTargetInfo = {
        .texture = pSwapchainTexture,
        .clear_color = SDL_FColor{ .r = 0.1f, .g = 0.15f, .b = 0.2f, .a = 1.0f },
        .load_op     = SDL_GPU_LOADOP_CLEAR,
        .store_op    = SDL_GPU_STOREOP_STORE
    };
    SDL_GPURenderPass* pRenderPass = SDL_BeginGPURenderPass(pCommandBuffer, &colorTargetInfo, 1, nullptr);

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(pRenderPass, m_pPipeline);

    // Uniform buffer
    SDL_PushGPUVertexUniformData(pCommandBuffer, 0, &m_MVP, sizeof(m_MVP));

    for (auto& it : m_meshes) {
        DrawMesh(it.second, pRenderPass);
    }

    SDL_EndGPURenderPass(pRenderPass);

    SDL_SubmitGPUCommandBuffer(pCommandBuffer);
}

void Renderer::SetViewMatrix(const glm::mat4& viewMat) {
    m_MVP.view = viewMat;
}

bool Renderer::CreateMesh(const MeshData& meshData, const string& meshName) {
    if (m_meshes.contains(meshName))
        return false;

    Mesh& mesh = m_meshes[meshName] = Mesh();

    SDL_GPUCommandBuffer* pCmdBuf = SDL_AcquireGPUCommandBuffer(m_pDevice);

    // Create vertex buffer
    mesh.pVertexBuffer = CreateBuffer(
        pCmdBuf,
        meshData.vertices.data(),
        meshData.vertices.size() * sizeof(Vertex),
        SDL_GPU_BUFFERUSAGE_VERTEX
    );

    // Create index buffer
    mesh.pIndexBuffer = CreateBuffer(
        pCmdBuf,
        meshData.indices.data(),
        meshData.indices.size() * sizeof(Index),
        SDL_GPU_BUFFERUSAGE_INDEX
    );
    mesh.indicesNum = meshData.indices.size();

    // Create textures
    for (i32 i = 0; i < TextureCount; i++)
        mesh.textures[i] = CreateTexture(pCmdBuf, meshData.texturesData[i]);

    SDL_SubmitGPUCommandBuffer(pCmdBuf);

    return true;
}

bool Renderer::DeleteMesh(const string& meshName) {
    if (!m_meshes.contains(meshName))
        return false;
    m_meshes.erase(meshName);
    return true;
}

Renderer::Mesh* Renderer::GetMesh(const string& meshName) {
    if (!m_meshes.contains(meshName))
        return nullptr;
    return &m_meshes[meshName];
}

bool Renderer::AddLight(const PointLight& light, const string& lightName) {
    if (m_pointLights.contains(lightName))
        return false;
    m_pointLights[lightName] = light;
    return true;
}

Renderer::PointLight* Renderer::GetLight(const string& lightName) {
    if (!m_pointLights.contains(lightName))
        return nullptr;
    return &m_pointLights[lightName];
}

SDL_GPUShader* Renderer::CreateShader(const string& source, SDL_GPUShaderStage shaderStage, const ShaderInfo& shaderInfo) {
    SDL_GPUShaderCreateInfo vertShaderCreateInfo = {
        .code_size            = source.size(),
        .code                 = (u8*)source.c_str(),
        .entrypoint           = "main",
        .format               = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage                = shaderStage,
        .num_samplers         = shaderInfo.numSamplers,
        .num_storage_textures = shaderInfo.numStorageTextures,
        .num_storage_buffers  = shaderInfo.numStorageBuffers,
        .num_uniform_buffers  = shaderInfo.numUniformBuffers,
    };

    SDL_GPUShader* pShader = SDL_CreateGPUShader(m_pDevice, &vertShaderCreateInfo);

    if (pShader == nullptr)
        FatalError("Could not create shader");

    return pShader; 
}

SDL_GPUGraphicsPipeline* Renderer::CreateGraphicsPipeline(const string& vertSrc, const ShaderInfo& vertInfo, const string& fragSrc, const ShaderInfo& fragInfo) {
    SDL_GPUShader* pVertShader = CreateShader(vertSrc, SDL_GPU_SHADERSTAGE_VERTEX, vertInfo);
    SDL_GPUShader* pFragShader = CreateShader(fragSrc, SDL_GPU_SHADERSTAGE_FRAGMENT, fragInfo);

    SDL_GPUColorTargetDescription colorTargetDesc = {};
    colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(m_pDevice, m_pWindow);
    colorTargetDesc.blend_state.enable_blend = true;
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
    pipelineCreateInfo.target_info.num_color_targets                 = 1;
    pipelineCreateInfo.target_info.color_target_descriptions         = &colorTargetDesc;
    pipelineCreateInfo.vertex_input_state.num_vertex_buffers         = 1;
    pipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions = &vertBufferDesc;
    pipelineCreateInfo.vertex_input_state.num_vertex_attributes      = s_vertexAttribs.size();
    pipelineCreateInfo.vertex_input_state.vertex_attributes          = s_vertexAttribs.data();
    pipelineCreateInfo.primitive_type                                = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineCreateInfo.vertex_shader                                 = pVertShader;
    pipelineCreateInfo.fragment_shader                               = pFragShader;

    SDL_GPUGraphicsPipeline* pPipeline = SDL_CreateGPUGraphicsPipeline(m_pDevice, &pipelineCreateInfo);
    if (pPipeline == nullptr)
        FatalError("Could not create graphics pipeline");

    return pPipeline;
}

SDL_GPUTransferBuffer* Renderer::CreateUploadBuffer(u32 size) {
    SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size  = size,
        .props = 0
    };
    SDL_GPUTransferBuffer* pTransferBuffer = SDL_CreateGPUTransferBuffer(m_pDevice, &transferBufferCreateInfo);
    if (pTransferBuffer == nullptr)
        Error("Could not create transfer buffer");
    return pTransferBuffer;
}

SDL_GPUBuffer* Renderer::CreateBuffer(SDL_GPUCommandBuffer* pCmdBuf, const void* pData, u32 dataSize, u32 usage) {
    SDL_GPUBuffer* pBuffer;
    SDL_GPUBufferCreateInfo bufCreateInfo = {
        .usage = usage,
        .size  = dataSize,
        .props = 0
    };

    pBuffer = SDL_CreateGPUBuffer(m_pDevice, &bufCreateInfo);
    if (pBuffer == nullptr)
        Error("Could not create buffer");

    UploadToBuffer(pCmdBuf, pBuffer, pData, dataSize);

    return pBuffer;
}

void Renderer::UploadToBuffer(SDL_GPUCommandBuffer* pCmdBuf, SDL_GPUBuffer* pBuffer, const void* data, u32 dataSize) {
    SDL_GPUTransferBuffer* pTransferBuffer = CreateUploadBuffer(dataSize);

    void* pMappedData = SDL_MapGPUTransferBuffer(m_pDevice, pTransferBuffer, false);
    SDL_memcpy(pMappedData, data, dataSize);
    SDL_UnmapGPUTransferBuffer(m_pDevice, pTransferBuffer);

    SDL_GPUTransferBufferLocation transferBufferLocation = {
        .transfer_buffer = pTransferBuffer,
        .offset          = 0
    };
    SDL_GPUBufferRegion bufferRegion = {
        .buffer = pBuffer,
        .offset = 0,
        .size = dataSize
    };

    SDL_GPUCopyPass* pCopyPass = SDL_BeginGPUCopyPass(pCmdBuf);
    SDL_UploadToGPUBuffer(pCopyPass, &transferBufferLocation, &bufferRegion, false);
    SDL_EndGPUCopyPass(pCopyPass);
}

SDL_GPUSampler* Renderer::CreateSampler() {
    SDL_GPUSamplerCreateInfo samplerCreateInfo = {
        .min_filter     = SDL_GPU_FILTER_LINEAR,
        .mag_filter     = SDL_GPU_FILTER_LINEAR,
        .mipmap_mode    = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    };
    SDL_GPUSampler* pSampler = SDL_CreateGPUSampler(m_pDevice, &samplerCreateInfo);
    if (pSampler == nullptr)
        FatalError("Could not create sampler");

    return pSampler;
}

SDL_GPUTexture* Renderer::CreateTexture(SDL_GPUCommandBuffer* pCmdBuf, const TextureData& texData) {
    SDL_GPUTexture* pTexture;

    SDL_GPUTextureCreateInfo createInfo = {
        .type                 = SDL_GPU_TEXTURETYPE_2D,
        .format               = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width                = texData.width,
        .height               = texData.height,
        .layer_count_or_depth = 1,
        .num_levels           = 1
    };

    pTexture = SDL_CreateGPUTexture(m_pDevice, &createInfo);
    if (pTexture == nullptr)
        Error("Could not create texture");

    UploadToTexture(pCmdBuf, pTexture, texData);

    return pTexture;
}

void Renderer::UploadToTexture(SDL_GPUCommandBuffer* pCmdBuf, SDL_GPUTexture* pTexture, const TextureData& texData) {
    const u32 byteSize = texData.width * texData.height * sizeof(u32); // NOTE: This assumes 32 bbp
    SDL_GPUTransferBuffer* pTransferBuffer = CreateUploadBuffer(byteSize);

    void* pMappedData = SDL_MapGPUTransferBuffer(m_pDevice, pTransferBuffer, false);
    SDL_memcpy(pMappedData, texData.pPixels, byteSize);
    SDL_UnmapGPUTransferBuffer(m_pDevice, pTransferBuffer);

    SDL_GPUTextureTransferInfo transferInfo = {
        .transfer_buffer = pTransferBuffer,
        .offset          = 0,
        .pixels_per_row  = 0,
        .rows_per_layer  = 0
    };

    SDL_GPUTextureRegion textureRegion = {
        .texture   = pTexture,
        .mip_level = 0,
        .layer     = 0,
        .x         = 0,
        .y         = 0,
        .w         = texData.width,
        .h         = texData.height,
        .d         = 1
    };

    SDL_GPUCopyPass* pCopyPass = SDL_BeginGPUCopyPass(pCmdBuf);
    SDL_UploadToGPUTexture(pCopyPass, &transferInfo, &textureRegion, false);
    SDL_EndGPUCopyPass(pCopyPass);
}

void Renderer::DrawMesh(const Mesh& mesh, SDL_GPURenderPass* pRenderPass) {
    // Binding sampler-texture pairs
    array<SDL_GPUTextureSamplerBinding, TextureCount> samplerBindings;
    for (i32 i = 0; i < TextureCount; i++) {
        SDL_GPUTextureSamplerBinding binding = {
            .texture = mesh.textures[i],
            .sampler = m_pSampler
        };
        samplerBindings[i] = binding;
    }
    SDL_BindGPUFragmentSamplers(pRenderPass, 0, samplerBindings.data(), TextureCount);

    // Binding vertex buffer
    SDL_GPUBufferBinding vertBufferBinding = {
        .buffer = mesh.pVertexBuffer,
        .offset = 0
    };
    SDL_BindGPUVertexBuffers(pRenderPass, 0, &vertBufferBinding, 1);

    // Binding index buffer
    SDL_GPUBufferBinding indexBufferBinding = {
        .buffer = mesh.pIndexBuffer,
        .offset = 0
    };
    SDL_BindGPUIndexBuffer(pRenderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    SDL_DrawGPUIndexedPrimitives(pRenderPass, mesh.indicesNum, 1, 0, 0, 0);
}

