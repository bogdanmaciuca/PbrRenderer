#include "renderer.h"

#include "../pch.h"

Renderer::Renderer(SDL_Window* pWindow) : m_pWindow(pWindow) {
    bool debugMode = true;
    m_pDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, debugMode, nullptr);
    if (m_pDevice == nullptr)
        FatalError("Could not create GPU device");

    if (SDL_ClaimWindowForGPUDevice(m_pDevice, pWindow) == false)
        FatalError("Could not claim window for GPU device");
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

    // Create textures
    for (i32 i = 0; i < MeshData::TextureCount; i++)
        mesh.textures[i] = CreateTexture(pCmdBuf, meshData.texturesData[i]);

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
    return nullptr;
}

SDL_GPUTexture* Renderer::CreateTexture(SDL_GPUCommandBuffer* pCmdBuf, const TextureData& texData) {
    SDL_GPUTexture* pTexture;

    SDL_GPUTextureCreateInfo createInfo = {
        .type   = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage  = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width  = texData.width,
        .height = texData.height,
        .layer_count_or_depth = 1,
        .num_levels = 1
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
