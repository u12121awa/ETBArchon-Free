#pragma once
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <d3d12.h>
#include <dxgi1_4.h>

namespace
{
    bool Initialized = false;
    HWND GameWindow = nullptr;
    WNDPROC WndProc = nullptr;

    UINT NumFrames = 0;
    ID3D12Device* Pd3dDevice = nullptr;
    ID3D12CommandQueue* CommandQueue = nullptr;
    ID3D12DescriptorHeap* Pd3dRtvDescHeap = nullptr;
    ID3D12DescriptorHeap* Pd3dSrvDescHeap = nullptr;
    ID3D12GraphicsCommandList* Pd3dCommandList = nullptr;
    std::vector<ID3D12CommandAllocator*> FrameCommandAllocators;
    UINT FrameIndex = 0;

    ID3D12Fence* PFence = nullptr;
    HANDLE FenceEvent = nullptr;
    UINT64 FenceLastSignaledValue = 0;
    std::vector<UINT64> FrameFenceValues;

    typedef HRESULT(APIENTRY* Present_t)(IDXGISwapChain3*, UINT, UINT);
    Present_t oPresent = nullptr;
    
    typedef void(STDMETHODCALLTYPE* tExecuteCommandLists)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);
    tExecuteCommandLists oExecuteCommandLists = nullptr;
}

namespace DX12Hook
{
    bool Initialize();
    void Shutdown();
    inline HWND GetGameWindow() { return GameWindow; };
};
