#pragma once

#include <d3d12.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <windows.h>

#include "stb_image.h"

struct TextureData
{
    ID3D12Resource* pTextureResource = nullptr;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuTextureHandle = {};
    int32_t nImageWidth = 0;
    int32_t nImageHeight = 0;
};

namespace TextureUtil
{
    inline UINT GetTexturePitch(UINT width)
    {
        return (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
    }
    
    inline bool LoadTextureFromMemory(
        ID3D12Device* pDevice,
        ID3D12CommandQueue* pCmdQueue,
        ID3D12CommandAllocator* pCmdAlloc,
        stbi_uc const* filename, int size,
        D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu_handle,
        int* out_width, int* out_height,
        ID3D12Resource** ppTextureResource)
    {
        int image_width = 0;
        int image_height = 0;
        unsigned char* image_data = stbi_load_from_memory(filename, size, &image_width, &image_height, NULL, 4);
        if (image_data == NULL) return false;

        D3D12_HEAP_PROPERTIES defaultHeapProps = {};
        defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = image_width;
        texDesc.Height = image_height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        ID3D12Resource* pTexture = NULL;
        HRESULT hr = pDevice->CreateCommittedResource(
            &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&pTexture)
        );
        if (FAILED(hr))
        {
            stbi_image_free(image_data);
            return false;
        }

        UINT uploadPitch = GetTexturePitch(image_width);
        UINT uploadSize = image_height * uploadPitch;

        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = uploadSize;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ID3D12Resource* uploadBuffer = NULL;
        hr = pDevice->CreateCommittedResource(
            &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer)
        );
        if (FAILED(hr))
        {
            stbi_image_free(image_data);
            pTexture->Release();
            return false;
        }

        void* mapped = NULL;
        D3D12_RANGE range = {0, uploadSize};
        uploadBuffer->Map(0, &range, &mapped);

        for (int y = 0; y < image_height; y++)
        {
            memcpy((void*)((uintptr_t)mapped + (uintptr_t)y * uploadPitch),
                   image_data + (uintptr_t)y * image_width * 4,
                   (size_t)image_width * 4);
        }

        uploadBuffer->Unmap(0, &range);
        stbi_image_free(image_data);

        ID3D12GraphicsCommandList* pCmdList = NULL;
        hr = pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCmdAlloc, NULL, IID_PPV_ARGS(&pCmdList));
        if (FAILED(hr))
        {
            uploadBuffer->Release();
            pTexture->Release();
            return false;
        }

        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        srcLocation.pResource = uploadBuffer;
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srcLocation.PlacedFootprint.Footprint.Width = image_width;
        srcLocation.PlacedFootprint.Footprint.Height = image_height;
        srcLocation.PlacedFootprint.Footprint.Depth = 1;
        srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        dstLocation.pResource = pTexture;
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = 0;

        pCmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, NULL);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = pTexture;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        pCmdList->ResourceBarrier(1, &barrier);

        pCmdList->Close();

        ID3D12CommandList* ppCommandLists[] = { pCmdList };
        pCmdQueue->ExecuteCommandLists(1, ppCommandLists);
        
        ID3D12Fence* fence = NULL;
        pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        HANDLE eventHandle = CreateEvent(0, 0, 0, 0);

        pCmdQueue->Signal(fence, 1);
        if (fence->GetCompletedValue() < 1)
        {
            fence->SetEventOnCompletion(1, eventHandle);
            WaitForSingleObject(eventHandle, INFINITE);
        }

        CloseHandle(eventHandle);
        fence->Release();
        pCmdList->Release();
        uploadBuffer->Release();

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        pDevice->CreateShaderResourceView(pTexture, &srvDesc, srv_cpu_handle);

        *out_width = image_width;
        *out_height = image_height;
        if (ppTextureResource) *ppTextureResource = pTexture;

        return true;
    }
    
    inline bool LoadImageFromMemoryAuto(
        ID3D12Device* pDevice,
        ID3D12DescriptorHeap* pd3dSrvDescHeap,
        ID3D12CommandQueue* pCmdQueue,
        ID3D12CommandAllocator* pCmdAlloc,
        stbi_uc const* bytes,
        int size,
        TextureData& texture,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuStart,
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart,
        UINT srvDescSize,
        UINT& srvOffset
    )
    {
        if (!pDevice || !pd3dSrvDescHeap || !pCmdQueue || !pCmdAlloc) return false;
        if (!bytes || size <= 0) return false;
        if (srvDescSize == 0) return false;
        
        const UINT idx = srvOffset++;
        D3D12_CPU_DESCRIPTOR_HANDLE cpu = srvCpuStart;
        cpu.ptr += (SIZE_T)idx * srvDescSize;

        D3D12_GPU_DESCRIPTOR_HANDLE gpu = srvGpuStart;
        gpu.ptr += (SIZE_T)idx * srvDescSize;

        texture.gpuTextureHandle = gpu;
        
        const bool ok = LoadTextureFromMemory(
            pDevice, pCmdQueue, pCmdAlloc,
            bytes, size,
            cpu,
            &texture.nImageWidth, &texture.nImageHeight,
            &texture.pTextureResource
        );

        if (!ok)
        {
            texture.gpuTextureHandle = {};
            texture.nImageWidth = texture.nImageHeight = 0;
            texture.pTextureResource = nullptr;
            return false;
        }

        return true;
    }
    
    inline void LoadImageFromMemory(
        ID3D12Device* pDevice,
        ID3D12DescriptorHeap* pd3dSrvDescHeap,
        ID3D12CommandQueue* pCmdQueue,
        ID3D12CommandAllocator* pCmdAlloc,
        stbi_uc const* filename,
        int size,
        TextureData& texture,
        int32_t nDescriptorIndex)
    {
        uint32_t handle_increment = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_CPU_DESCRIPTOR_HANDLE my_cpu_handle = pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
        my_cpu_handle.ptr += (handle_increment * (SIZE_T)nDescriptorIndex);

        texture.gpuTextureHandle = pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
        texture.gpuTextureHandle.ptr += (handle_increment * (SIZE_T)nDescriptorIndex);

        LoadTextureFromMemory(pDevice, pCmdQueue, pCmdAlloc, filename, size,
                              my_cpu_handle,
                              &texture.nImageWidth, &texture.nImageHeight,
                              &texture.pTextureResource);
    }
    

    inline bool LoadTextureFromRawRGBA(
        ID3D12Device* pDevice,
        ID3D12DescriptorHeap* pSrvHeap,
        ID3D12CommandQueue* pCmdQueue,
        ID3D12CommandAllocator* pCmdAlloc,
        const unsigned char* pixel_data, 
        int width, 
        int height,
        D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu_handle,
        ID3D12Resource** ppTextureResource)
    {
        if (pDevice == nullptr || pixel_data == nullptr || width <= 0 || height <= 0) 
            return false;
        
        D3D12_HEAP_PROPERTIES defaultHeapProps = {};
        defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        ID3D12Resource* pTexture = nullptr;
        if (FAILED(pDevice->CreateCommittedResource(
            &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pTexture))))
        {
            return false;
        }
        
        UINT uploadPitch = GetTexturePitch(width);
        UINT uploadSize = height * uploadPitch;

        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = uploadSize;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ID3D12Resource* uploadBuffer = nullptr;
        if (FAILED(pDevice->CreateCommittedResource(
            &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer))))
        {
            pTexture->Release();
            return false;
        }
        
        void* mapped = nullptr;
        D3D12_RANGE range = { 0, uploadSize };
        if (SUCCEEDED(uploadBuffer->Map(0, &range, &mapped)))
        {
            for (int y = 0; y < height; y++)
            {
                memcpy((void*)((uintptr_t)mapped + (uintptr_t)y * uploadPitch),
                       pixel_data + (uintptr_t)y * width * 4,
                       (size_t)width * 4);
            }
            uploadBuffer->Unmap(0, &range);
        }
        
        ID3D12GraphicsCommandList* pCmdList = nullptr;
        if (FAILED(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCmdAlloc, nullptr, IID_PPV_ARGS(&pCmdList))))
        {
            uploadBuffer->Release();
            pTexture->Release();
            return false;
        }

        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        srcLocation.pResource = uploadBuffer;
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srcLocation.PlacedFootprint.Footprint.Width = width;
        srcLocation.PlacedFootprint.Footprint.Height = height;
        srcLocation.PlacedFootprint.Footprint.Depth = 1;
        srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        dstLocation.pResource = pTexture;
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = 0;

        pCmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
        
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = pTexture;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        pCmdList->ResourceBarrier(1, &barrier);

        pCmdList->Close();
        
        ID3D12CommandList* ppCommandLists[] = { pCmdList };
        pCmdQueue->ExecuteCommandLists(1, ppCommandLists);
        
        ID3D12Fence* fence = nullptr;
        if (SUCCEEDED(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
        {
            HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (eventHandle)
            {
                pCmdQueue->Signal(fence, 1);
                if (fence->GetCompletedValue() < 1)
                {
                    fence->SetEventOnCompletion(1, eventHandle);
                    WaitForSingleObject(eventHandle, INFINITE);
                }
                CloseHandle(eventHandle);
            }
            fence->Release();
        }

        pCmdList->Release();
        uploadBuffer->Release();
        
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        
        pDevice->CreateShaderResourceView(pTexture, &srvDesc, srv_cpu_handle);
        
        if (ppTextureResource) *ppTextureResource = pTexture;

        return true;
    }

    inline bool LoadRawRGBAAuto(
        ID3D12Device* pDevice,
        ID3D12DescriptorHeap* pd3dSrvDescHeap,
        ID3D12CommandQueue* pCmdQueue,
        ID3D12CommandAllocator* pCmdAlloc,
        const unsigned char* rgbaData,
        int width, int height,
        TextureData& texture,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuStart,
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart,
        UINT srvDescSize,
        UINT& srvOffset
    )
    {
        if (!pDevice || !rgbaData || width <= 0 || height <= 0) return false;
        
        const UINT idx = srvOffset++;
        
        D3D12_CPU_DESCRIPTOR_HANDLE cpu = srvCpuStart;
        cpu.ptr += (SIZE_T)idx * srvDescSize;

        D3D12_GPU_DESCRIPTOR_HANDLE gpu = srvGpuStart;
        gpu.ptr += (SIZE_T)idx * srvDescSize;
        
        texture.gpuTextureHandle = gpu;
        texture.nImageWidth = width;
        texture.nImageHeight = height;
        
        const bool ok = LoadTextureFromRawRGBA(
            pDevice, pd3dSrvDescHeap, pCmdQueue, pCmdAlloc,
            rgbaData, width, height,
            cpu,
            &texture.pTextureResource
        );

        if (!ok)
        {
            texture.gpuTextureHandle = {};
            texture.nImageWidth = texture.nImageHeight = 0;
            texture.pTextureResource = nullptr;
            return false;
        }

        return true;
    }
    __declspec(noinline) inline TextureData* LoadImmediate(const unsigned char* data, size_t size,
                                           ID3D12Device* pDevice, 
                                           ID3D12DescriptorHeap* pSrvHeap,
                                           ID3D12CommandQueue* pCmdQueue, 
                                           ID3D12CommandAllocator* pCmdAlloc, 
                                           D3D12_CPU_DESCRIPTOR_HANDLE srvCpuStart, 
                                           D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart, 
                                           UINT srvDescSize, UINT& srvOffset) 
    {
        TextureData* pNewTexture = new TextureData();
        
        bool ok = LoadImageFromMemoryAuto(
            pDevice, 
            pSrvHeap, 
            pCmdQueue, 
            pCmdAlloc,
            data, 
            (int)size, 
            *pNewTexture,
            srvCpuStart, 
            srvGpuStart, 
            srvDescSize, 
            srvOffset
        );
        
        if (!ok) {
            delete pNewTexture;
            return nullptr;
        }
        
        return pNewTexture;
    }
}
