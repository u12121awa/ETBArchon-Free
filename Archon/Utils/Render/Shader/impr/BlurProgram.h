#pragma once

#include "d3dx12.h"
#include "Shader/DX12Shader.h"

struct MCBlurParams
{
    float InputResolution[2];
    float uSize[2];

    float uLocation[2];
    float radius;
    float Brightness;

    float Quality;
    float _pad0;
    float _pad1;
    float _pad2;
    float color1[4];
};

class BlurProgram : public DX12ShaderBase
{
private:
    DXGI_FORMAT m_Format = DXGI_FORMAT_UNKNOWN;
    ComPtr<ID3D12Resource> m_InputTexture;
    int m_Width = 0, m_Height = 0;

    ID3D12DescriptorHeap* m_SharedSrvHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE m_BlurSrvCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_BlurSrvGpu = {};
    bool m_HasSrvSlot = false;

    ComPtr<ID3D12RootSignature> m_MaskRootSig;
    ComPtr<ID3D12PipelineState> m_MaskPSO;

    ComPtr<ID3D12PipelineState> m_PSO_NoBlur;
    ComPtr<ID3D12PipelineState> m_MaskPSO_NoBlur;

    bool m_EnableBlur = true;

    std::string GetBlurSource()
    {
        std::string s;
        s.reserve(4096);

        s += xorstr_("cbuffer Constants : register(b0) {\n");
        s += xorstr_("    float2 InputResolution;\n");
        s += xorstr_("    float2 uSize;\n");
        s += xorstr_("    float2 uLocation;\n");
        s += xorstr_("    float radius;\n");
        s += xorstr_("    float Brightness;\n");
        s += xorstr_("    float Quality;\n");
        s += xorstr_("    float3 _padding;\n");
        s += xorstr_("    float4 color1;\n");
        s += xorstr_("};\n");

        s += xorstr_("Texture2D InputTexture : register(t0);\n");
        s += xorstr_("SamplerState InputSampler : register(s0);\n");

        s += xorstr_("struct PSInput {\n");
        s += xorstr_("    float4 Pos : SV_POSITION;\n");
        s += xorstr_("    float2 UV  : TEXCOORD0;\n");
        s += xorstr_("};\n");

        s += xorstr_("PSInput VSMain(uint vID : SV_VertexID) {\n");
        s += xorstr_("    PSInput output;\n");
        s += xorstr_("    output.UV = float2((vID << 1) & 2, vID & 2);\n");
        s += xorstr_("    output.Pos = float4(output.UV * float2(2, -2) + float2(-1, 1), 0, 1);\n");
        s += xorstr_("    return output;\n");
        s += xorstr_("}\n");

        s += xorstr_("float roundedBoxSDF(float2 p, float2 b, float r) {\n");
        s += xorstr_("    float2 q = abs(p) - b + r;\n");
        s += xorstr_("    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;\n");
        s += xorstr_("}\n");

        s += xorstr_("float4 BlurLogic(float2 uv) {\n");
        s += xorstr_("    const float TAU = 6.28318530718;\n");
        s += xorstr_("    float2 BlurRadius = (Quality + 2.0) / InputResolution;\n");
        s += xorstr_("    float4 Color = InputTexture.Sample(InputSampler, uv);\n");
        s += xorstr_("    float stepVal = TAU / 16.0;\n");
        s += xorstr_("    for (float d = 0.0; d < TAU; d += stepVal) {\n");
        s += xorstr_("        float cosD = cos(d);\n");
        s += xorstr_("        float sinD = sin(d);\n");
        s += xorstr_("        for (float i = 0.2; i <= 1.0; i += 0.2) {\n");
        s += xorstr_("            float2 offset = float2(cosD, sinD) * BlurRadius * i;\n");
        s += xorstr_("            Color += InputTexture.Sample(InputSampler, uv + offset);\n");
        s += xorstr_("        }\n");
        s += xorstr_("    }\n");
        s += xorstr_("    Color /= 80.0;\n");
        s += xorstr_("    return (Color + color1) * Brightness;\n");
        s += xorstr_("}\n");

        s += xorstr_("float4 PSMain(PSInput input) : SV_Target {\n");
        s += xorstr_("    float2 halfSize = uSize * 0.5;\n");
        s += xorstr_("    float2 centerPos = uLocation + halfSize;\n");
        s += xorstr_("    float dist = roundedBoxSDF(input.Pos.xy - centerPos, halfSize, radius);\n");
        s += xorstr_("    float smoothedAlpha = 1.0 - smoothstep(0.0, 1.0, dist);\n");
        s += xorstr_("    if (smoothedAlpha <= 0.00001) discard;\n");
        s += xorstr_("    float2 currentUV = input.Pos.xy / InputResolution;\n");
        s += xorstr_("    float4 blurredColor = BlurLogic(currentUV);\n");
        s += xorstr_("    return float4(blurredColor.rgb, smoothedAlpha);\n");
        s += xorstr_("}\n");

        return s;
    }

    std::string GetNoBlurSource()
    {
        std::string s;
        s.reserve(4096);

        s += xorstr_("cbuffer Constants : register(b0) {\n");
        s += xorstr_("    float2 InputResolution;\n");
        s += xorstr_("    float2 uSize;\n");
        s += xorstr_("    float2 uLocation;\n");
        s += xorstr_("    float radius;\n");
        s += xorstr_("    float Brightness;\n");
        s += xorstr_("    float Quality;\n");
        s += xorstr_("    float3 _padding;\n");
        s += xorstr_("    float4 color1;\n");
        s += xorstr_("};\n");

        s += xorstr_("Texture2D InputTexture : register(t0);\n");
        s += xorstr_("SamplerState InputSampler : register(s0);\n");

        s += xorstr_("struct PSInput {\n");
        s += xorstr_("    float4 Pos : SV_POSITION;\n");
        s += xorstr_("    float2 UV  : TEXCOORD0;\n");
        s += xorstr_("};\n");

        s += xorstr_("PSInput VSMain(uint vID : SV_VertexID) {\n");
        s += xorstr_("    PSInput output;\n");
        s += xorstr_("    output.UV = float2((vID << 1) & 2, vID & 2);\n");
        s += xorstr_("    output.Pos = float4(output.UV * float2(2, -2) + float2(-1, 1), 0, 1);\n");
        s += xorstr_("    return output;\n");
        s += xorstr_("}\n");

        s += xorstr_("float roundedBoxSDF(float2 p, float2 b, float r) {\n");
        s += xorstr_("    float2 q = abs(p) - b + r;\n");
        s += xorstr_("    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;\n");
        s += xorstr_("}\n");

        s += xorstr_("float4 PSMain(PSInput input) : SV_Target {\n");
        s += xorstr_("    float2 halfSize = uSize * 0.5;\n");
        s += xorstr_("    float2 centerPos = uLocation + halfSize;\n");
        s += xorstr_("    float dist = roundedBoxSDF(input.Pos.xy - centerPos, halfSize, radius);\n");
        s += xorstr_("    float smoothedAlpha = 1.0 - smoothstep(0.0, 1.0, dist);\n");
        s += xorstr_("    if (smoothedAlpha <= 0.00001) discard;\n");
        s += xorstr_("    float2 uv = input.Pos.xy / InputResolution;\n");
        s += xorstr_("    float4 c = InputTexture.Sample(InputSampler, uv);\n");
        s += xorstr_("    c = (c + color1) * Brightness;\n");
        s += xorstr_("    return float4(c.rgb, smoothedAlpha);\n");
        s += xorstr_("}\n");

        return s;
    }

    std::string GetMaskedBlurSource()
    {
        std::string s;
        s.reserve(4096);

        s += xorstr_("cbuffer Constants : register(b0) {\n");
        s += xorstr_("    float2 InputResolution;\n");
        s += xorstr_("    float2 uSize;\n");
        s += xorstr_("    float2 uLocation;\n");
        s += xorstr_("    float radius;\n");
        s += xorstr_("    float Brightness;\n");
        s += xorstr_("    float Quality;\n");
        s += xorstr_("    float3 _padding;\n");
        s += xorstr_("    float4 color1;\n");
        s += xorstr_("};\n");

        s += xorstr_("Texture2D InputTexture : register(t0);\n");
        s += xorstr_("Texture2D MaskTexture  : register(t1);\n");
        s += xorstr_("SamplerState InputSampler : register(s0);\n");

        s += xorstr_("struct PSInput {\n");
        s += xorstr_("    float4 Pos : SV_POSITION;\n");
        s += xorstr_("    float2 UV  : TEXCOORD0;\n");
        s += xorstr_("};\n");

        s += xorstr_("PSInput VSMain(uint vID : SV_VertexID) {\n");
        s += xorstr_("    PSInput output;\n");
        s += xorstr_("    output.UV = float2((vID << 1) & 2, vID & 2);\n");
        s += xorstr_("    output.Pos = float4(output.UV * float2(2, -2) + float2(-1, 1), 0, 1);\n");
        s += xorstr_("    return output;\n");
        s += xorstr_("}\n");

        s += xorstr_("float4 BlurLogic(float2 uv) {\n");
        s += xorstr_("    const float TAU = 6.28318530718;\n");
        s += xorstr_("    float2 BlurRadius = (Quality + 2.0) / InputResolution;\n");
        s += xorstr_("    float4 Color = InputTexture.Sample(InputSampler, uv);\n");
        s += xorstr_("    float stepVal = TAU / 16.0;\n");
        s += xorstr_("    for (float d = 0.0; d < TAU; d += stepVal) {\n");
        s += xorstr_("        float cosD = cos(d);\n");
        s += xorstr_("        float sinD = sin(d);\n");
        s += xorstr_("        for (float i = 0.2; i <= 1.0; i += 0.2) {\n");
        s += xorstr_("            float2 offset = float2(cosD, sinD) * BlurRadius * i;\n");
        s += xorstr_("            Color += InputTexture.Sample(InputSampler, uv + offset);\n");
        s += xorstr_("        }\n");
        s += xorstr_("    }\n");
        s += xorstr_("    Color /= 80.0;\n");
        s += xorstr_("    return (Color + color1) * Brightness;\n");
        s += xorstr_("}\n");

        s += xorstr_("float4 PSMain(PSInput input) : SV_Target {\n");
        s += xorstr_("    float2 minP = uLocation - 1.0;\n");
        s += xorstr_("    float2 maxP = uLocation + uSize + 1.0;\n");
        s += xorstr_("    if (input.Pos.x < minP.x || input.Pos.y < minP.y || input.Pos.x > maxP.x || input.Pos.y > maxP.y) discard;\n");
        s += xorstr_("    float2 uv = input.Pos.xy / InputResolution;\n");
        s += xorstr_("    float maskAlpha = MaskTexture.SampleLevel(InputSampler, uv, 0).r;\n");
        s += xorstr_("    float4 blurredColor = BlurLogic(uv);\n");
        s += xorstr_("    return float4(blurredColor.rgb, maskAlpha);\n");
        s += xorstr_("}\n");

        return s;
    }

    std::string GetMaskedNoBlurSource()
    {
        std::string s;
        s.reserve(4096);

        s += xorstr_("cbuffer Constants : register(b0) {\n");
        s += xorstr_("    float2 InputResolution;\n");
        s += xorstr_("    float2 uSize;\n");
        s += xorstr_("    float2 uLocation;\n");
        s += xorstr_("    float radius;\n");
        s += xorstr_("    float Brightness;\n");
        s += xorstr_("    float Quality;\n");
        s += xorstr_("    float3 _padding;\n");
        s += xorstr_("    float4 color1;\n");
        s += xorstr_("};\n");

        s += xorstr_("Texture2D InputTexture : register(t0);\n");
        s += xorstr_("Texture2D MaskTexture  : register(t1);\n");
        s += xorstr_("SamplerState InputSampler : register(s0);\n");

        s += xorstr_("struct PSInput {\n");
        s += xorstr_("    float4 Pos : SV_POSITION;\n");
        s += xorstr_("    float2 UV  : TEXCOORD0;\n");
        s += xorstr_("};\n");

        s += xorstr_("PSInput VSMain(uint vID : SV_VertexID) {\n");
        s += xorstr_("    PSInput output;\n");
        s += xorstr_("    output.UV = float2((vID << 1) & 2, vID & 2);\n");
        s += xorstr_("    output.Pos = float4(output.UV * float2(2, -2) + float2(-1, 1), 0, 1);\n");
        s += xorstr_("    return output;\n");
        s += xorstr_("}\n");

        s += xorstr_("float4 PSMain(PSInput input) : SV_Target {\n");
        s += xorstr_("    float2 minP = uLocation - 1.0;\n");
        s += xorstr_("    float2 maxP = uLocation + uSize + 1.0;\n");
        s += xorstr_("    if (input.Pos.x < minP.x || input.Pos.y < minP.y || input.Pos.x > maxP.x || input.Pos.y > maxP.y) discard;\n");
        s += xorstr_("    float2 uv = input.Pos.xy / InputResolution;\n");
        s += xorstr_("    float maskAlpha = MaskTexture.SampleLevel(InputSampler, uv, 0).r;\n");
        s += xorstr_("    float4 c = InputTexture.Sample(InputSampler, uv);\n");
        s += xorstr_("    c = (c + color1) * Brightness;\n");
        s += xorstr_("    return float4(c.rgb, maskAlpha);\n");
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

    void SetEnableBlur(bool enable)
    {
        m_EnableBlur = enable;
    }

    bool InitBlur(ID3D12Device* device,
                  DXGI_FORMAT format,
                  ID3D12DescriptorHeap* sharedSrvHeap,
                  D3D12_CPU_DESCRIPTOR_HANDLE srvCpuStart,
                  D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart,
                  UINT srvDescSize,
                  UINT* pSrvCurrentOffset)
    {
        Init(device, format);

        m_SharedSrvHeap = sharedSrvHeap;

        if (pSrvCurrentOffset && srvDescSize != 0)
        {
            const UINT idx = (*pSrvCurrentOffset)++;
            m_BlurSrvCpu.ptr = srvCpuStart.ptr + (SIZE_T)idx * srvDescSize;
            m_BlurSrvGpu.ptr = srvGpuStart.ptr + (SIZE_T)idx * srvDescSize;
            m_HasSrvSlot = true;
        }
        else
        {
            m_HasSrvSlot = false;
        }

        {
            CD3DX12_DESCRIPTOR_RANGE texTable;
            texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

            CD3DX12_ROOT_PARAMETER params[2];
            params[0].InitAsConstants(sizeof(MCBlurParams) / 4, 0);
            params[1].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

            D3D12_STATIC_SAMPLER_DESC sampler = {};
            sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            sampler.ShaderRegister = 0;

            CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
            rootSigDesc.Init(2, params, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

            ComPtr<ID3DBlob> signature, error;
            D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
            if (error) OutputDebugStringA((char*)error->GetBufferPointer());

            m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                          IID_PPV_ARGS(&m_RootSig));

            auto vsBlob = Compile(GetCommonVS(), xorstr_("VSMain"), xorstr_("vs_5_0"));

            auto psBlur = Compile(GetBlurSource(), xorstr_("PSMain"), xorstr_("ps_5_0"));
            auto psNoBlur = Compile(GetNoBlurSource(), xorstr_("PSMain"), xorstr_("ps_5_0"));
            if (!psBlur || !psNoBlur) return false;

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature = m_RootSig.Get();
            psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
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

            psoDesc.DepthStencilState.DepthEnable = FALSE;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = m_Format;
            psoDesc.SampleDesc.Count = 1;

            psoDesc.PS = CD3DX12_SHADER_BYTECODE(psBlur.Get());
            m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO));

            psoDesc.PS = CD3DX12_SHADER_BYTECODE(psNoBlur.Get());
            m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO_NoBlur));
        }

        {
            CD3DX12_DESCRIPTOR_RANGE t0;
            t0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
            CD3DX12_DESCRIPTOR_RANGE t1;
            t1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

            CD3DX12_ROOT_PARAMETER params[3];
            params[0].InitAsConstants(sizeof(MCBlurParams) / 4, 0);
            params[1].InitAsDescriptorTable(1, &t0, D3D12_SHADER_VISIBILITY_PIXEL);
            params[2].InitAsDescriptorTable(1, &t1, D3D12_SHADER_VISIBILITY_PIXEL);

            D3D12_STATIC_SAMPLER_DESC sampler = {};
            sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            sampler.ShaderRegister = 0;

            CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
            rootSigDesc.Init(3, params, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

            ComPtr<ID3DBlob> signature, error;
            D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
            if (error) OutputDebugStringA((char*)error->GetBufferPointer());

            m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                          IID_PPV_ARGS(&m_MaskRootSig));

            auto vsBlob = Compile(GetCommonVS(), xorstr_("VSMain"), xorstr_("vs_5_0"));

            auto psBlur = Compile(GetMaskedBlurSource(), xorstr_("PSMain"), xorstr_("ps_5_0"));
            auto psNoBlur = Compile(GetMaskedNoBlurSource(), xorstr_("PSMain"), xorstr_("ps_5_0"));
            if (!psBlur || !psNoBlur) return false;

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature = m_MaskRootSig.Get();
            psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
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

            psoDesc.DepthStencilState.DepthEnable = FALSE;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = m_Format;
            psoDesc.SampleDesc.Count = 1;

            psoDesc.PS = CD3DX12_SHADER_BYTECODE(psBlur.Get());
            m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_MaskPSO));

            psoDesc.PS = CD3DX12_SHADER_BYTECODE(psNoBlur.Get());
            m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_MaskPSO_NoBlur));
        }

        m_Initialized = true;
        return true;
    }

    void UpdateSize(int width, int height)
    {
        if (width <= 0 || height <= 0) return;
        if (m_Width == width && m_Height == height && m_InputTexture) return;

        m_Width = width;
        m_Height = height;
        m_InputTexture.Reset();

        auto heapDef = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_Format, width, height, 1, 1);

        m_Device->CreateCommittedResource(&heapDef, D3D12_HEAP_FLAG_NONE, &texDesc,
                                          D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr,
                                          IID_PPV_ARGS(&m_InputTexture));

        if (m_HasSrvSlot)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = m_Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            m_Device->CreateShaderResourceView(m_InputTexture.Get(), &srvDesc, m_BlurSrvCpu);
        }
    }

    void RenderToBackBuffer(ID3D12GraphicsCommandList* cmdList,
                            ID3D12Resource* backBuffer,
                            D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV,
                            CD3DX12_RECT sc,
                            MCBlurParams& params)
    {
        if (!m_Initialized || !m_InputTexture || !m_HasSrvSlot) return;

        D3D12_RESOURCE_BARRIER bars[2];
        bars[0] = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer,
                                                       D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                       D3D12_RESOURCE_STATE_COPY_SOURCE);
        bars[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_InputTexture.Get(),
                                                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                       D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList->ResourceBarrier(2, bars);

        cmdList->CopyResource(m_InputTexture.Get(), backBuffer);

        bars[0] = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer,
                                                       D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                       D3D12_RESOURCE_STATE_RENDER_TARGET);
        bars[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_InputTexture.Get(),
                                                       D3D12_RESOURCE_STATE_COPY_DEST,
                                                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(2, bars);

        cmdList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);

        cmdList->SetPipelineState(m_EnableBlur ? m_PSO.Get() : m_PSO_NoBlur.Get());
        cmdList->SetGraphicsRootSignature(m_RootSig.Get());
        cmdList->SetGraphicsRootDescriptorTable(1, m_BlurSrvGpu);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &params, 0);

        CD3DX12_VIEWPORT vp(0.0f, 0.0f, (float)m_Width, (float)m_Height);
        cmdList->RSSetViewports(1, &vp);
        cmdList->RSSetScissorRects(1, &sc);

        DrawQuad(cmdList);
    }

    void RenderMasked(ID3D12GraphicsCommandList* cmdList,
                      ID3D12Resource* backBuffer,
                      D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV,
                      CD3DX12_RECT sc,
                      MCBlurParams& params,
                      D3D12_GPU_DESCRIPTOR_HANDLE maskSrvGpu)
    {
        if (!m_Initialized || !m_InputTexture || !m_HasSrvSlot) return;

        D3D12_RESOURCE_BARRIER bars[2];
        bars[0] = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer,
                                                       D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                       D3D12_RESOURCE_STATE_COPY_SOURCE);
        bars[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_InputTexture.Get(),
                                                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                       D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList->ResourceBarrier(2, bars);

        cmdList->CopyResource(m_InputTexture.Get(), backBuffer);

        bars[0] = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer,
                                                       D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                       D3D12_RESOURCE_STATE_RENDER_TARGET);
        bars[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_InputTexture.Get(),
                                                       D3D12_RESOURCE_STATE_COPY_DEST,
                                                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(2, bars);

        cmdList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);

        cmdList->SetPipelineState(m_EnableBlur ? m_MaskPSO.Get() : m_MaskPSO_NoBlur.Get());
        cmdList->SetGraphicsRootSignature(m_MaskRootSig.Get());
        cmdList->SetGraphicsRootDescriptorTable(1, m_BlurSrvGpu);
        cmdList->SetGraphicsRootDescriptorTable(2, maskSrvGpu);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &params, 0);

        CD3DX12_VIEWPORT vp(0.0f, 0.0f, (float)m_Width, (float)m_Height);
        cmdList->RSSetViewports(1, &vp);
        cmdList->RSSetScissorRects(1, &sc);

        DrawQuad(cmdList);
    }
};
