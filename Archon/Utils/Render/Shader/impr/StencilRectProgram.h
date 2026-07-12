#pragma once

#include "../DX12Shader.h"
#include <string>

#include "../../d3dx12.h"

struct StencilParams
{
    float colorTL[4];
    float colorTR[4];
    float colorBL[4];
    float colorBR[4];
    float uSize[2];
    float uLocation[2];
    float _padding[20];
};

class StencilRectProgram : public DX12ShaderBase
{
private:
    std::string GetGradientMaskShader()
    {
        std::string s;
        s.reserve(1024);

        s += xorstr_("cbuffer Constants : register(b0) {\n");
        s += xorstr_("    float4 cTL, cTR, cBL, cBR;\n");
        s += xorstr_("    float2 uSize;\n");
        s += xorstr_("    float2 uLocation;\n");
        s += xorstr_("};\n");
        
        s += xorstr_("Texture2D    g_Texture : register(t0);\n");
        s += xorstr_("SamplerState g_Sampler : register(s0);\n");

        s += xorstr_("struct PSInput {\n");
        s += xorstr_("    float4 Pos : SV_POSITION;\n");
        s += xorstr_("};\n");
        
        s += xorstr_("float3 createGradient(float2 coords, float3 c1, float3 c2, float3 c3, float3 c4) {\n");
        s += xorstr_("    float3 top = lerp(c1, c2, coords.x);\n");
        s += xorstr_("    float3 bot = lerp(c4, c3, coords.x);\n"); 
        s += xorstr_("    return lerp(top, bot, coords.y);\n");
        s += xorstr_("}\n");
        
        s += xorstr_("float4 PSMain(PSInput input) : SV_Target {\n");
        s += xorstr_("    float2 uv = (input.Pos.xy - uLocation) / uSize;\n");
        s += xorstr_("    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)\n");
        s += xorstr_("        discard;\n");
        s += xorstr_("    float4 texColor = g_Texture.Sample(g_Sampler, uv);\n");
        s += xorstr_("    float maskAlpha = texColor.a;\n");
        s += xorstr_("    float3 gradientRGB = createGradient(uv, cTL.rgb, cTR.rgb, cBL.rgb, cBR.rgb);\n");
        s += xorstr_("    float finalAlpha = maskAlpha * cTL.a;\n");
        s += xorstr_("    if (finalAlpha <= 0.001) discard;\n");
        s += xorstr_("    return float4(gradientRGB, finalAlpha);\n");
        s += xorstr_("}\n");

        return s;
    }

public:
    bool Init(ID3D12Device* device, DXGI_FORMAT format) override
    {
        m_Device = device;
        CD3DX12_ROOT_PARAMETER params[2];

        params[0].InitAsConstants(sizeof(StencilParams) / 4, 0);

        CD3DX12_DESCRIPTOR_RANGE texRange;
        texRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        params[1].InitAsDescriptorTable(1, &texRange, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC sampler(
            0,
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP
        );

        CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
        rootSigDesc.Init(2, params, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
        auto psBlob = Compile(GetGradientMaskShader(), xorstr_("PSMain"), xorstr_("ps_5_0"));

        if (!psBlob)
        {
            MessageBoxA(NULL, xorstr_("Stencil Shader Compile Failed!"), xorstr_("Error"), MB_OK);
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

    void Render(ID3D12GraphicsCommandList* cmdList,
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                CD3DX12_RECT sc,
                StencilParams& params,
                D3D12_GPU_DESCRIPTOR_HANDLE maskSrvHandle)
    {
        if (!m_Initialized) return;

        cmdList->SetPipelineState(m_PSO.Get());
        cmdList->SetGraphicsRootSignature(m_RootSig.Get());
        cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        cmdList->SetGraphicsRoot32BitConstants(0, sizeof(StencilParams) / 4, &params, 0);

        if (maskSrvHandle.ptr != 0)
        {
            cmdList->SetGraphicsRootDescriptorTable(1, maskSrvHandle);
        }

        CD3DX12_VIEWPORT vp(0.0f, 0.0f, sc.right, sc.bottom);
        cmdList->RSSetViewports(1, &vp);
        cmdList->RSSetScissorRects(1, &sc);

        DrawQuad(cmdList);
    }
};
