#pragma once

#include <wrl.h>
#include <string>
#include <vector>
#include "../DX12Shader.h"
#include "../../d3dx12.h"

using Microsoft::WRL::ComPtr;

struct MaskInstance
{
    float x, y, w, h;
    float radius;
    float _pad[3];
};

class MeshProgram : public DX12ShaderBase
{
public:
    bool Init(ID3D12Device* device, DXGI_FORMAT format) override
    {
        m_Device = device;
        m_Format = format;
        return true;
    }

    bool InitMesh(ID3D12Device* device,
                  DXGI_FORMAT maskFormat,
                  ID3D12DescriptorHeap* sharedSrvHeap,
                  D3D12_CPU_DESCRIPTOR_HANDLE srvCpuStart,
                  D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart,
                  UINT srvDescSize,
                  UINT* pSrvCurrentOffset,
                  D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuStart,
                  UINT rtvDescSize,
                  UINT* pRtvCurrentOffset)
    {
        Init(device, maskFormat);
        m_SharedSrvHeap = sharedSrvHeap;

        if (!pSrvCurrentOffset || !pRtvCurrentOffset) return false;

        UINT idx = (*pSrvCurrentOffset)++;
        m_InstanceSrvCpu.ptr = srvCpuStart.ptr + (SIZE_T)idx * srvDescSize;
        m_InstanceSrvGpu.ptr = srvGpuStart.ptr + (SIZE_T)idx * srvDescSize;

        idx = (*pSrvCurrentOffset)++;
        m_MaskSrvCpu.ptr = srvCpuStart.ptr + (SIZE_T)idx * srvDescSize;
        m_MaskSrvGpu.ptr = srvGpuStart.ptr + (SIZE_T)idx * srvDescSize;
        m_HasMaskSrv = true;

        UINT ridx = (*pRtvCurrentOffset)++;
        m_MaskRTV.ptr = rtvCpuStart.ptr + (SIZE_T)ridx * rtvDescSize;

        CD3DX12_DESCRIPTOR_RANGE range;
        range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        CD3DX12_ROOT_PARAMETER rp[2];
        rp[0].InitAsConstants(4, 0);
        rp[1].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_ALL);

        CD3DX12_ROOT_SIGNATURE_DESC rsd;
        rsd.Init(2, rp, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> sig, err;
        D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err);
        m_Device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&m_RootSig));

        auto vs = Compile(GetVS(), xorstr_("VSMain"), xorstr_("vs_5_0"));
        auto ps = Compile(GetPS(), xorstr_("PSMain"), xorstr_("ps_5_0"));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
        pso.pRootSignature = m_RootSig.Get();
        pso.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
        pso.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
        pso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        pso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

        D3D12_BLEND_DESC bd = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        bd.RenderTarget[0].BlendEnable = TRUE;
        bd.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        bd.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        bd.RenderTarget[0].BlendOp = D3D12_BLEND_OP_MAX;
        bd.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        bd.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
        bd.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        pso.BlendState = bd;
        pso.RTVFormats[0] = m_Format;
        pso.NumRenderTargets = 1;
        pso.SampleDesc.Count = 4;
        pso.SampleDesc.Quality = 0;
        pso.SampleMask = UINT_MAX;
        pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso.DepthStencilState.DepthEnable = FALSE;
        pso.DepthStencilState.StencilEnable = FALSE;

        m_Device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&m_PSO));
        m_Initialized = true;
        return true;
    }

    void UpdateSize(int width, int height)
    {
        if (!m_Initialized || width <= 0 || height <= 0) return;
        if (m_Width == width && m_Height == height && m_MaskTextureMSAA && m_MaskTextureResolve) return;

        m_Width = width;
        m_Height = height;

        m_MaskTextureMSAA.Reset();
        m_MaskTextureResolve.Reset();

        auto heapDef = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        {
            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(m_Format, width, height, 1, 1);
            desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            desc.SampleDesc.Count = 4;
            desc.SampleDesc.Quality = 0;

            float cv[4] = {0.f, 0.f, 0.f, 0.f};
            D3D12_CLEAR_VALUE clr = {};
            clr.Format = m_Format;
            memcpy(clr.Color, cv, 16);

            m_Device->CreateCommittedResource(
                &heapDef,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                &clr,
                IID_PPV_ARGS(&m_MaskTextureMSAA));

            m_Device->CreateRenderTargetView(m_MaskTextureMSAA.Get(), nullptr, m_MaskRTV);
        }

        {
            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(m_Format, width, height, 1, 1);
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;

            m_Device->CreateCommittedResource(
                &heapDef,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_PPV_ARGS(&m_MaskTextureResolve));

            D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
            srv.Format = m_Format;
            srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srv.Texture2D.MipLevels = 1;
            srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            m_Device->CreateShaderResourceView(m_MaskTextureResolve.Get(), &srv, m_MaskSrvCpu);
        }
    }

    void UploadInstances(const MaskInstance* instances, uint32_t count)
    {
        if (!m_Initialized || !instances || count == 0) return;
        UINT64 bytes = (UINT64)count * sizeof(MaskInstance);
        EnsureUpload(bytes);
        if (m_Mapped) memcpy(m_Mapped, instances, (size_t)bytes);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
        srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv.Format = DXGI_FORMAT_UNKNOWN;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Buffer.FirstElement = 0;
        srv.Buffer.NumElements = count;
        srv.Buffer.StructureByteStride = sizeof(MaskInstance);
        srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        m_Device->CreateShaderResourceView(m_InstanceUpload.Get(), &srv, m_InstanceSrvCpu);
    }

    void DrawMask(ID3D12GraphicsCommandList* cmd, const CD3DX12_RECT& scissor, int screenW, int screenH,
                  uint32_t instanceCount)
    {
        if (!m_Initialized || !m_MaskTextureMSAA || !m_MaskTextureResolve) return;
        
        {
            auto b = CD3DX12_RESOURCE_BARRIER::Transition(
                m_MaskTextureResolve.Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RESOLVE_DEST);
            cmd->ResourceBarrier(1, &b);
        }
        
        cmd->OMSetRenderTargets(1, &m_MaskRTV, FALSE, nullptr);

        float clearColor[4] = {0.f, 0.f, 0.f, 0.f};
        cmd->ClearRenderTargetView(m_MaskRTV, clearColor, 0, nullptr);

        ID3D12DescriptorHeap* heaps[] = {m_SharedSrvHeap};
        cmd->SetDescriptorHeaps(1, heaps);

        cmd->SetPipelineState(m_PSO.Get());
        cmd->SetGraphicsRootSignature(m_RootSig.Get());

        float c[4] = {(float)screenW, (float)screenH, 0.f, 0.f};
        cmd->SetGraphicsRoot32BitConstants(0, 4, c, 0);
        cmd->SetGraphicsRootDescriptorTable(1, m_InstanceSrvGpu);

        CD3DX12_VIEWPORT vp(0.f, 0.f, (float)screenW, (float)screenH);
        cmd->RSSetViewports(1, &vp);
        cmd->RSSetScissorRects(1, &scissor);

        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->DrawInstanced(6, instanceCount, 0, 0);
        
        {
            auto b = CD3DX12_RESOURCE_BARRIER::Transition(
                m_MaskTextureMSAA.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
            cmd->ResourceBarrier(1, &b);
        }
        
        cmd->ResolveSubresource(
            m_MaskTextureResolve.Get(), 0,
            m_MaskTextureMSAA.Get(), 0,
            m_Format);
        
        {
            D3D12_RESOURCE_BARRIER barriers[2];
            
            barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
                m_MaskTextureResolve.Get(),
                D3D12_RESOURCE_STATE_RESOLVE_DEST,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            
            barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
                m_MaskTextureMSAA.Get(),
                D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET);

            cmd->ResourceBarrier(2, barriers);
        }
    }
    D3D12_GPU_DESCRIPTOR_HANDLE GetMaskSRVGpu() const { return m_MaskSrvGpu; }

private:
    void EnsureUpload(UINT64 bytes)
    {
        if (m_InstanceUpload && m_UploadBytes >= bytes) return;
        if (m_InstanceUpload) m_InstanceUpload->Unmap(0, nullptr);
        m_InstanceUpload.Reset();
        m_UploadBytes = (UINT64)std::max<UINT64>(bytes, 4096);

        auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto buf = CD3DX12_RESOURCE_DESC::Buffer(m_UploadBytes);

        m_Device->CreateCommittedResource(
            &heap,
            D3D12_HEAP_FLAG_NONE,
            &buf,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_InstanceUpload));

        m_InstanceUpload->Map(0, nullptr, (void**)&m_Mapped);
    }

    std::string GetVS()
    {
        std::string s = "";
        s += xorstr_("cbuffer Config : register(b0) \n");
        s += xorstr_("{ \n");
        s += xorstr_("    float2 Resolution; \n");
        s += xorstr_("    float2 Padding; \n");
        s += xorstr_("}; \n");

        s += xorstr_("struct InstanceData \n");
        s += xorstr_("{ \n");
        s += xorstr_("    float4 Rect; \n");
        s += xorstr_("    float Radius; \n");
        s += xorstr_("    float3 Padding2; \n");
        s += xorstr_("}; \n");

        s += xorstr_("StructuredBuffer<InstanceData> Instances : register(t0); \n");

        s += xorstr_("struct VSOutput \n");
        s += xorstr_("{ \n");
        s += xorstr_("    float4 Pos : SV_POSITION; \n");
        s += xorstr_("    float2 LocalPos : TEXCOORD0; \n");
        s += xorstr_("    nointerpolation uint InstanceID : TEXCOORD1; \n");
        s += xorstr_("}; \n");

        s += xorstr_("VSOutput VSMain(uint vID : SV_VertexID, uint iID : SV_InstanceID) \n");
        s += xorstr_("{ \n");
        s += xorstr_("    float2 Quad[6] = \n");
        s += xorstr_("    { \n");
        s += xorstr_("        float2(0, 0), float2(1, 0), float2(1, 1), \n");
        s += xorstr_("        float2(0, 0), float2(1, 1), float2(0, 1) \n");
        s += xorstr_("    }; \n");

        s += xorstr_("    InstanceData inst = Instances[iID]; \n");
        s += xorstr_("    float expand = 1.0; \n");
        s += xorstr_("    float2 pos0 = inst.Rect.xy - float2(expand, expand); \n");
        s += xorstr_("    float2 size = inst.Rect.zw + float2(expand * 2.0, expand * 2.0); \n");
        s += xorstr_("    float2 worldPos = pos0 + Quad[vID] * size; \n");

        s += xorstr_("    VSOutput output; \n");
        s += xorstr_("    output.LocalPos = worldPos; \n");
        s += xorstr_("    output.InstanceID = iID; \n");

        s += xorstr_("    float2 ndc = (worldPos / Resolution) * float2(2, -2) + float2(-1, 1); \n");
        s += xorstr_("    output.Pos = float4(ndc, 0, 1); \n");

        s += xorstr_("    return output; \n");
        s += xorstr_("}\n");
        return s;
    }
    std::string GetPS()
    {
        std::string s = "";
        s += xorstr_("cbuffer Config : register(b0) \n");
        s += xorstr_("{ \n");
        s += xorstr_("    float2 Resolution; \n");
        s += xorstr_("    float2 Padding; \n");
        s += xorstr_("}; \n");

        s += xorstr_("struct InstanceData \n");
        s += xorstr_("{ \n");
        s += xorstr_("    float4 Rect; \n");
        s += xorstr_("    float Radius; \n");
        s += xorstr_("    float3 Padding2; \n");
        s += xorstr_("}; \n");

        s += xorstr_("StructuredBuffer<InstanceData> Instances : register(t0); \n");

        s += xorstr_("struct VSOutput \n");
        s += xorstr_("{ \n");
        s += xorstr_("    float4 Pos : SV_POSITION; \n");
        s += xorstr_("    float2 LocalPos : TEXCOORD0; \n");
        s += xorstr_("    nointerpolation uint InstanceID : TEXCOORD1; \n");
        s += xorstr_("}; \n");

        s += xorstr_("float roundedBoxSDF(float2 p, float2 b, float r) \n");
        s += xorstr_("{ \n");
        s += xorstr_("    float2 q = abs(p) - b + r; \n");
        s += xorstr_("    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r; \n");
        s += xorstr_("} \n");

        s += xorstr_("float PSMain(VSOutput input) : SV_Target \n");
        s += xorstr_("{ \n");
        s += xorstr_("    InstanceData inst = Instances[input.InstanceID]; \n");
        s += xorstr_("    float2 halfSize = inst.Rect.zw * 0.5; \n");
        s += xorstr_("    float2 center = inst.Rect.xy + halfSize; \n");
        s += xorstr_("    float dist = roundedBoxSDF(input.LocalPos - center, halfSize, inst.Radius); \n");
        s += xorstr_("    float a = 1.0 - smoothstep(0.0, 1.0, dist); \n");
        s += xorstr_("    return a; \n");
        s += xorstr_("}\n");
        return s;
    }



    DXGI_FORMAT m_Format = DXGI_FORMAT_UNKNOWN;
    int m_Width = 0, m_Height = 0;
    bool m_Initialized = false;

    ID3D12DescriptorHeap* m_SharedSrvHeap = nullptr;
    ComPtr<ID3D12RootSignature> m_RootSig;
    ComPtr<ID3D12PipelineState> m_PSO;

    D3D12_CPU_DESCRIPTOR_HANDLE m_InstanceSrvCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_InstanceSrvGpu = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_MaskSrvCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_MaskSrvGpu = {};
    bool m_HasMaskSrv = false;

    D3D12_CPU_DESCRIPTOR_HANDLE m_MaskRTV = {};
    ComPtr<ID3D12Resource> m_MaskTextureMSAA;
    ComPtr<ID3D12Resource> m_MaskTextureResolve;

    ComPtr<ID3D12Resource> m_InstanceUpload;
    uint8_t* m_Mapped = nullptr;
    UINT64 m_UploadBytes = 0;
};
