#pragma once

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <iostream>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

#pragma comment(lib, "dxguid.lib")

using Microsoft::WRL::ComPtr;

class DX12ShaderBase
{
protected:
    ComPtr<ID3D12Device> m_Device;
    ComPtr<ID3D12PipelineState> m_PSO;
    ComPtr<ID3D12RootSignature> m_RootSig;
    bool m_Initialized = false;
    
    std::string GetCommonVS() 
    {
        std::string shader;
        shader.reserve(512); 
        
        shader += xorstr_("struct PSInput { float4 Pos : SV_POSITION; float2 UV : TEXCOORD0; };\n");
        shader += xorstr_("PSInput VSMain(uint vID : SV_VertexID) {\n");
        shader += xorstr_("    PSInput output;\n");
        shader += xorstr_("    output.UV = float2((vID << 1) & 2, vID & 2);\n");
        shader += xorstr_("    output.Pos = float4(output.UV * float2(2, -2) + float2(-1, 1), 0, 1);\n");
        shader += xorstr_("    return output;\n");
        shader += xorstr_("}\n");

        return shader;
    }

    ComPtr<ID3DBlob> Compile(const std::string& source, const std::string& entry, const std::string& profile)
    {
        ComPtr<ID3DBlob> blob, error;
        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        HRESULT hr = D3DCompile(source.c_str(), source.length(), nullptr, nullptr, nullptr, entry.c_str(),
                                profile.c_str(), flags, 0, &blob, &error);
        if (FAILED(hr) && error) OutputDebugStringA((char*)error->GetBufferPointer());
        return blob;
    }

    ComPtr<ID3DBlob> CompileFromFile(const std::wstring& path, const std::string& entry, const std::string& profile)
    {
        ComPtr<ID3DBlob> blob, error;
        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        HRESULT hr = D3DCompileFromFile(path.c_str(), nullptr, nullptr, entry.c_str(), profile.c_str(), flags, 0, &blob,
                                        &error);
        if (FAILED(hr))
        {
            if (error) OutputDebugStringA((char*)error->GetBufferPointer());
            else OutputDebugStringA(xorstr_("Failed to load shader file."));
        }
        return blob;
    }

public:
    virtual ~DX12ShaderBase() = default;
    virtual bool Init(ID3D12Device* device, DXGI_FORMAT format) = 0;

    void DrawQuad(ID3D12GraphicsCommandList* cmdList)
    {
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }
    
};
