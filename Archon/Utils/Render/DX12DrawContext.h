#pragma once

#include <d3d12.h>

struct DX12DrawContext
{
    ID3D12GraphicsCommandList* CommandList; 
    ID3D12Resource* BackBuffer; 
    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandle;
    
    D3D12_GPU_DESCRIPTOR_HANDLE ScreenCaptureSRV; 
    
    ImDrawList* WindowsDrawList;
    ImDrawList* BackGroundDrawList;
    
    float ScreenWidth;
    float ScreenHeight;

    ImVec2 WindowsPos = ImVec2(0, 0);
    ImVec2 WindowsSize = ImVec2(0, 0);

    float DeltaTime;
};
