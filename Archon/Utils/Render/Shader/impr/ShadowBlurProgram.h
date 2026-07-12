#pragma once

#include "d3dx12.h"
#include "Shader/DX12Shader.h"
#include <map>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstring>

struct ShadowBlurParams
{
    float rect[4];
    float color[4];
};

struct CachedShadow
{
    ComPtr<ID3D12Resource> texture;
    ComPtr<ID3D12Resource> uploadBuffer;
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle{};
    bool isValid = false;
};

class ShadowBlurProgram : public DX12ShaderBase
{
private:
    DXGI_FORMAT m_Format = DXGI_FORMAT_UNKNOWN;

    struct Kernel
    {
        int width = 0;
        std::vector<float> data;
    };
    
    static int Clamp(int c)
    {
        return (c < 0) ? 0 : (c > 255 ? 255 : c);
    }

    static Kernel MakeKernel(float radius)
    {
        int r = (int)std::ceil(radius);
        int rows = r * 2 + 1;
        std::vector<float> matrix(rows);

        float sigma = radius / 3.0f;
        if (sigma <= 0.0001f) sigma = 0.0001f;

        float sigma22 = 2.0f * sigma * sigma;
        float sigmaPi2 = 6.2831855f * sigma;
        float sqrtSigmaPi2 = std::sqrt(sigmaPi2);
        float radius2 = radius * radius;

        float total = 0.0f;
        int index = 0;

        for (int row = -r; row <= r; row++)
        {
            float distance = (float)(row * row);
            if (distance > radius2) matrix[index] = 0.0f;
            else matrix[index] = std::exp(-distance / sigma22) / sqrtSigmaPi2;

            total += matrix[index];
            index++;
        }

        if (total <= 0.0f) total = 1.0f;
        for (int i = 0; i < rows; i++) matrix[i] /= total;

        Kernel k;
        k.width = rows;
        k.data = std::move(matrix);
        return k;
    }

    static void ConvolveAndTranspose(const Kernel& kernel,
                                     const std::vector<uint32_t>& inPixels,
                                     std::vector<uint32_t>& outPixels,
                                     int width, int height,
                                     bool alpha, bool premultiply, bool unpremultiply)
    {
        const float* matrix = kernel.data.data();
        int cols = kernel.width;
        int cols2 = cols / 2;

        for (int y = 0; y < height; y++)
        {
            int index = y;
            int ioffset = y * width;

            for (int x = 0; x < width; x++)
            {
                float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;

                for (int col = -cols2; col <= cols2; col++)
                {
                    float f = matrix[cols2 + col];
                    if (f == 0.0f) continue;

                    int ix = x + col;
                    if (ix < 0) ix = 0;
                    else if (ix >= width) ix = width - 1;

                    uint32_t rgb = inPixels[ioffset + ix];

                    int pa = (rgb >> 24) & 0xFF;
                    int pr = (rgb >> 16) & 0xFF;
                    int pg = (rgb >> 8) & 0xFF;
                    int pb = rgb & 0xFF;

                    if (premultiply)
                    {
                        float a255 = pa * 0.003921569f;
                        pr = (int)(pr * a255);
                        pg = (int)(pg * a255);
                        pb = (int)(pb * a255);
                    }

                    a += f * (float)pa;
                    r += f * (float)pr;
                    g += f * (float)pg;
                    b += f * (float)pb;
                }

                if (unpremultiply && a != 0.0f && a != 255.0f)
                {
                    float f = 255.0f / a;
                    r *= f;
                    g *= f;
                    b *= f;
                }

                int ia = alpha ? Clamp((int)(a + 0.5f)) : 255;
                int ir = Clamp((int)(r + 0.5f));
                int ig = Clamp((int)(g + 0.5f));
                int ib = Clamp((int)(b + 0.5f));

                outPixels[index] = (uint32_t)((ia << 24) | (ir << 16) | (ig << 8) | ib);
                index += height; // transpose step
            }
        }
    }
    
    static float Saturate(float x) { return (x < 0.f) ? 0.f : (x > 1.f ? 1.f : x); }

    static float SmoothStep(float a, float b, float x)
    {
        float t = Saturate((x - a) / (b - a));
        return t * t * (3.f - 2.f * t);
    }

    static uint32_t PackWhite(uint8_t a)
    {
        return (uint32_t(a) << 24) | 0x00FFFFFF;
    }
    
    static float SdRoundRect(float px, float py, float x, float y, float w, float h, float r)
    {
        float cx = x + w * 0.5f;
        float cy = y + h * 0.5f;
        float hx = w * 0.5f;
        float hy = h * 0.5f;

        float qx = std::fabs(px - cx) - (hx - r);
        float qy = std::fabs(py - cy) - (hy - r);

        float ax = std::max(qx, 0.0f);
        float ay = std::max(qy, 0.0f);

        float outside = std::sqrt(ax * ax + ay * ay) - r;
        float inside = std::min(std::max(qx, qy), 0.0f);
        return outside + inside;
    }

    static void DrawRoundedRectMaskSDF(std::vector<uint32_t>& img, int W, int H,
                                       float x, float y, float w, float h, float radiusPx)
    {
        float r = std::max(0.0f, radiusPx);
        float maxR = 0.5f * std::min(w, h);
        if (r > maxR) r = maxR;
        
        const float aa = 1.5f;

        int x0 = std::max(0, (int)std::floor(x - aa - 1));
        int y0 = std::max(0, (int)std::floor(y - aa - 1));
        int x1 = std::min(W, (int)std::ceil (x + w + aa + 1));
        int y1 = std::min(H, (int)std::ceil (y + h + aa + 1));

        for (int iy = y0; iy < y1; iy++)
        {
            for (int ix = x0; ix < x1; ix++)
            {
                float px = ix + 0.5f;
                float py = iy + 0.5f;

                float d = SdRoundRect(px, py, x, y, w, h, r);
                
                float t = SmoothStep(-aa, +aa, d);
                float a = 1.0f - t;

                uint8_t A = (uint8_t)std::lround(Saturate(a) * 255.0f);
                if (A)
                    img[(size_t)iy * (size_t)W + (size_t)ix] = PackWhite(A);
            }
        }
    }
    
    std::vector<uint32_t> GenerateBlurredImage(int width, int height, float radius, int blurStrength)
    {
        std::vector<uint32_t> original((size_t)width * (size_t)height, 0x00000000);

        int innerW = width - blurStrength * 2;
        int innerH = height - blurStrength * 2;
        if (innerW < 0) innerW = 0;
        if (innerH < 0) innerH = 0;

        float cr = std::max(0.0f, radius);
        cr = std::min(cr, 0.5f * (float)std::min(innerW, innerH));
        
        float rx = (float)blurStrength;
        float ry = (float)blurStrength;
        float rw = (float)innerW;
        float rh = (float)innerH;

        DrawRoundedRectMaskSDF(original, width, height, rx, ry, rw, rh, cr);

        Kernel kernel = MakeKernel((float)blurStrength);
        std::vector<uint32_t> temp((size_t)width * (size_t)height);
        std::vector<uint32_t> result((size_t)width * (size_t)height);

        if (blurStrength > 0)
        {
            ConvolveAndTranspose(kernel, original, temp, width, height, true, true, false);
            ConvolveAndTranspose(kernel, temp, result, height, width, true, false, true);
        }
        else result = original;
        
        for (auto& pixel : result)
        {
            uint32_t a = (pixel >> 24) & 0xFF;
            uint32_t r = (pixel >> 16) & 0xFF;
            uint32_t g = (pixel >> 8) & 0xFF;
            uint32_t b = (pixel) & 0xFF;
            pixel = (a << 24) | (b << 16) | (g << 8) | r;
        }

        return result;
    }
    
    static uint64_t MakeCacheKey(int w, int h, int blurR, int cornerR)
    {
        return (uint64_t)(w & 0x1FFFFF)
            | ((uint64_t)(h & 0x1FFFFF) << 21)
            | ((uint64_t)(blurR & 0x3FF) << 42)
            | ((uint64_t)(cornerR & 0x3FF) << 52);
    }

    std::map<uint64_t, CachedShadow> m_ShadowCache;

    ID3D12DescriptorHeap* m_SrvHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE m_SrvCpuStart{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_SrvGpuStart{};
    UINT m_SrvDescSize = 0;

    UINT m_CurrentSrvOffset = 0;
    static constexpr UINT kMaxSrvSlots = 50;

    D3D12_GPU_DESCRIPTOR_HANDLE AllocateSrv(ID3D12Resource* tex)
    {
        if (!m_Device || !m_SrvHeap) return {};

        if (m_CurrentSrvOffset >= kMaxSrvSlots) m_CurrentSrvOffset = 0;

        D3D12_CPU_DESCRIPTOR_HANDLE cpu = m_SrvCpuStart;
        cpu.ptr += (SIZE_T)m_CurrentSrvOffset * (SIZE_T)m_SrvDescSize;

        D3D12_GPU_DESCRIPTOR_HANDLE gpu = m_SrvGpuStart;
        gpu.ptr += (SIZE_T)m_CurrentSrvOffset * (SIZE_T)m_SrvDescSize;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        m_Device->CreateShaderResourceView(tex, &srvDesc, cpu);

        m_CurrentSrvOffset++;
        return gpu;
    }

    std::string GetVS()
    {
        std::string s;
        s += xorstr_("cbuffer Params : register(b0) { float4 Rect; float4 Color; };\n");
        s += xorstr_("struct VSOut { float4 Pos : SV_POSITION; float2 UV : TEXCOORD0; };\n");
        s += xorstr_("VSOut VSMain(uint vID : SV_VertexID) {\n");
        s += xorstr_("    VSOut o;\n");
        s += xorstr_("    float u = (vID==1 || vID==4 || vID==5) ? 1.0 : 0.0;\n");
        s += xorstr_("    float v = (vID==2 || vID==3 || vID==5) ? 1.0 : 0.0;\n");
        s += xorstr_("    float2 uv = float2(u, v);\n");
        s += xorstr_("    o.UV = uv;\n");
        s += xorstr_("    float2 pos;\n");
        s += xorstr_("    pos.x = Rect.x + uv.x * Rect.z;\n");
        s += xorstr_("    pos.y = Rect.y - uv.y * Rect.w; // DX12: down is -Y\n");
        s += xorstr_("    o.Pos = float4(pos, 0, 1);\n");
        s += xorstr_("    return o;\n");
        s += xorstr_("}\n");
        return s;
    }



    std::string GetPS()
    {
        std::string s;
        s += xorstr_("cbuffer Params : register(b0) { float4 Rect; float4 Color; };\n");
        s += xorstr_("Texture2D Tex : register(t0);\n");
        s += xorstr_("SamplerState Samp : register(s0);\n");
        s += xorstr_("struct VSOut { float4 Pos : SV_POSITION; float2 UV : TEXCOORD0; };\n");
        s += xorstr_("float4 PSMain(VSOut input) : SV_Target {\n");
        s += xorstr_("    float4 texColor = Tex.Sample(Samp, input.UV);\n");
        s += xorstr_("    return texColor * Color;\n");
        s += xorstr_("}\n");
        return s;
    }

public:
    bool Init(ID3D12Device* device, DXGI_FORMAT format) override
    {
        m_Device = device;
        m_Format = format;
        return true;
    }

    bool InitShadow(ID3D12Device* device,
                    DXGI_FORMAT format,
                    ID3D12DescriptorHeap* srvHeap,
                    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart,
                    D3D12_GPU_DESCRIPTOR_HANDLE gpuStart,
                    UINT descSize)
    {
        Init(device, format);

        m_SrvHeap = srvHeap;
        m_SrvCpuStart = cpuStart;
        m_SrvGpuStart = gpuStart;
        m_SrvDescSize = descSize;
        m_CurrentSrvOffset = 0;
        
        CD3DX12_DESCRIPTOR_RANGE texTable;
        texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        CD3DX12_ROOT_PARAMETER params[2];
        params[0].InitAsConstants(8, 0);
        params[1].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;

        CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
        rootSigDesc.Init(2, params, 1, &sampler,
                         D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature, error;
        HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (error) OutputDebugStringA((char*)error->GetBufferPointer());
        if (FAILED(hr)) return false;

        hr = m_Device->CreateRootSignature(0,
                                           signature->GetBufferPointer(),
                                           signature->GetBufferSize(),
                                           IID_PPV_ARGS(&m_RootSig));
        if (FAILED(hr)) return false;

        auto vsBlob = Compile(GetVS(), xorstr_("VSMain"), xorstr_("vs_5_0"));
        auto psBlob = Compile(GetPS(), xorstr_("PSMain"), xorstr_("ps_5_0"));
        if (!vsBlob || !psBlob) return false;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_RootSig.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        
        D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.BlendState = blendDesc;
        
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;

        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_Format;
        psoDesc.SampleDesc.Count = 1;

        hr = m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO));
        if (FAILED(hr)) return false;

        m_Initialized = true;
        return true;
    }
    
    void DrawBlurredShadow(ID3D12GraphicsCommandList* cmdList,
                           float x, float y,
                           float width, float height,
                           float radius,
                           int blurStrength,
                           float color[4],
                           float screenW, float screenH)
    {
        if (!m_Initialized || !cmdList || !m_Device || !m_SrvHeap) return;
        
        CD3DX12_VIEWPORT screenViewport(0.0f, 0.0f, screenW, screenH);
        cmdList->RSSetViewports(1, &screenViewport);
        
        float ex = x - (float)blurStrength;
        float ey = y - (float)blurStrength;
        float ew = width + (float)blurStrength * 2.0f;
        float eh = height + (float)blurStrength * 2.0f;

        int iw = (int)std::ceil(ew);
        int ih = (int)std::ceil(eh);
        if (iw <= 0 || ih <= 0) return;
        
        int icorner = (int)std::lround(std::max(0.0f, radius));
        uint64_t key = MakeCacheKey(iw, ih, blurStrength, icorner);
        auto& cached = m_ShadowCache[key];

        if (!cached.isValid)
        {
            std::vector<uint32_t> pixels = GenerateBlurredImage(iw, ih, radius, blurStrength);

            auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM,
                                                        (UINT64)iw, (UINT)ih, 1, 1);

            HRESULT hr = m_Device->CreateCommittedResource(
                &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
                D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&cached.texture));
            if (FAILED(hr)) return;

            const UINT64 uploadBufferSize = GetRequiredIntermediateSize(cached.texture.Get(), 0, 1);
            auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

            hr = m_Device->CreateCommittedResource(
                &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cached.uploadBuffer));
            if (FAILED(hr)) return;

            D3D12_SUBRESOURCE_DATA subData = {};
            subData.pData = pixels.data();
            subData.RowPitch = (LONG_PTR)iw * 4;
            subData.SlicePitch = subData.RowPitch * ih;

            UpdateSubresources(cmdList, cached.texture.Get(), cached.uploadBuffer.Get(), 0, 0, 1, &subData);

            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                cached.texture.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            cmdList->ResourceBarrier(1, &barrier);

            cached.srvGpuHandle = AllocateSrv(cached.texture.Get());
            cached.isValid = true;
        }

        cmdList->SetPipelineState(m_PSO.Get());
        cmdList->SetGraphicsRootSignature(m_RootSig.Get());

        ID3D12DescriptorHeap* heaps[] = { m_SrvHeap };
        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->SetGraphicsRootDescriptorTable(1, cached.srvGpuHandle);
        
        float ndcX = (ex / screenW) * 2.0f - 1.0f;
        float ndcY = 1.0f - (ey / screenH) * 2.0f;
        float ndcW = ((float)iw / screenW) * 2.0f;
        float ndcH = ((float)ih / screenH) * 2.0f;

        ShadowBlurParams params;
        params.rect[0] = ndcX;
        params.rect[1] = ndcY;
        params.rect[2] = ndcW;
        params.rect[3] = ndcH;
        std::memcpy(params.color, color, sizeof(float) * 4);

        cmdList->SetGraphicsRoot32BitConstants(0, 8, &params, 0);

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(6, 1, 0, 0);

    }

    void ClearCache()
    {
        m_ShadowCache.clear();
        m_CurrentSrvOffset = 0;
    }
};
