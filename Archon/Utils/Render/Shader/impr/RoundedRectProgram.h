#pragma once
#include "d3dx12.h"
#include "Shader/DX12Shader.h"

struct RoundedRectParams
{
    float color1[4];
    float color2[4];
    float color3[4];
    float color4[4];
    float uSize[2];
    float uLocation[2];
    float radius;
    float _padding[3];
};

class RoundedRectProgram : public DX12ShaderBase
{
private:
    std::string GetShaderSource()
    {
        std::string s;
        s.reserve(1500);
        
        s += xorstr_("cbuffer Constants : register(b0) {\n");
        s += xorstr_("    float4 color1;\n");
        s += xorstr_("    float4 color2;\n");
        s += xorstr_("    float4 color3;\n");
        s += xorstr_("    float4 color4;\n");
        s += xorstr_("    float2 uSize;\n");
        s += xorstr_("    float2 uLocation;\n");
        s += xorstr_("    float  radius;\n");
        s += xorstr_("    float3 _padding;\n");
        s += xorstr_("};\n");
        
        s += xorstr_("struct PSInput {\n");
        s += xorstr_("    float4 Pos : SV_POSITION;\n");
        s += xorstr_("    float2 UV  : TEXCOORD0;\n");
        s += xorstr_("};\n");
        
        s += xorstr_("float roundedBoxSDF(float2 centerPos, float2 size, float r) {\n");
        s += xorstr_("    float2 q = abs(centerPos) - size + r;\n");
        s += xorstr_("    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;\n");
        s += xorstr_("}\n");
        
        s += xorstr_("float3 createGradient(float2 coords, float3 c1, float3 c2, float3 c3, float3 c4) {\n");
        s += xorstr_("    float3 left  = lerp(c1, c2, coords.y);\n");
        s += xorstr_("    float3 right = lerp(c3, c4, coords.y);\n");
        s += xorstr_("    float3 color = lerp(left, right, coords.x);\n");
        s += xorstr_("    float noise = frac(sin(dot(coords.xy, float2(12.9898, 78.233))) * 43758.5453);\n");
        s += xorstr_("    color += lerp(0.0019607843, -0.0019607843, noise);\n");
        s += xorstr_("    return color;\n");
        s += xorstr_("}\n");
        
        s += xorstr_("float4 PSMain(PSInput input) : SV_Target {\n");
        s += xorstr_("    float2 halfSize = uSize / 2.0;\n");
        s += xorstr_("    float2 centerPos = uLocation + halfSize;\n");
        s += xorstr_("    float dist = roundedBoxSDF(input.Pos.xy - centerPos, halfSize, radius);\n");
        s += xorstr_("    float smoothedAlpha = (1.0 - smoothstep(-0.5, 0.5, dist)) * color1.a;\n");
        s += xorstr_("    if (smoothedAlpha <= 0.001) discard;\n");
        s += xorstr_("    float2 normCoord = (input.Pos.xy - uLocation) / uSize;\n");
        s += xorstr_("    normCoord = clamp(normCoord, 0.0, 1.0);\n");
        s += xorstr_("    float3 finalRGB = createGradient(normCoord, color1.rgb, color2.rgb, color3.rgb, color4.rgb);\n");
        s += xorstr_("    return float4(finalRGB, smoothedAlpha);\n");
        s += xorstr_("}\n");

        return s;
    }

public:
    bool Init(ID3D12Device* device, DXGI_FORMAT format) override
    {
        m_Device = device;

        CD3DX12_ROOT_PARAMETER params[1];
        params[0].InitAsConstants(sizeof(RoundedRectParams) / 4, 0);

        CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
        rootSigDesc.Init(1, params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature, error;
        D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
            return false;
        }
        m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                      IID_PPV_ARGS(&m_RootSig));

        auto vsBlob = Compile(GetCommonVS(), xorstr_("VSMain"), xorstr_("vs_5_0"));
        auto psBlob = Compile(GetShaderSource(), xorstr_("PSMain"), xorstr_("ps_5_0"));

        if (!psBlob)
        {
            MessageBoxA(NULL, xorstr_("Rounded Rect Shader Compile Failed!"), xorstr_("Error"), MB_OK);
            return false;
        }

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
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.BlendState = blendDesc;

        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = format;
        psoDesc.SampleDesc.Count = 1;

        m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO));

        m_Initialized = true;
        return true;
    }

    void Render(ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, CD3DX12_RECT sc,
                RoundedRectParams& params)
    {
        if (!m_Initialized) return;

        cmdList->SetPipelineState(m_PSO.Get());
        cmdList->SetGraphicsRootSignature(m_RootSig.Get());
        cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        cmdList->SetGraphicsRoot32BitConstants(0, sizeof(RoundedRectParams) / 4, &params, 0);

        CD3DX12_VIEWPORT vp(0.0f, 0.0f, sc.right, sc.bottom);
        cmdList->RSSetViewports(1, &vp);
        cmdList->RSSetScissorRects(1, &sc);

        DrawQuad(cmdList);
    }
};
