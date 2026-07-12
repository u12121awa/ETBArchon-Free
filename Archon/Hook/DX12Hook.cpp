#include "pch.h"
#include "DX12Hook.h"

#include "Hooks.h"
#include "../Archon.h"
#include "../Core/Managers/FontManager.h"
#include "../Core/Module/features.h"
#include "../Utils/Minhook/MinHook.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx12.h"
#include "../UI/MCChatSystem.h"
#include "../Utils/Render/DX12DrawContext.h"
#include "../Utils/RawData/rawData_logo_Image.h"
#include "../Utils/Render/TextureUtil.h"
#include "../Utils/World/FTickCallbackSystem.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

inline std::atomic g_IsUnloading = false;

void WaitForFence(UINT64 fenceValue)
{
    if (!PFence || !FenceEvent)
        return;
    if (PFence->GetCompletedValue() < fenceValue)
    {
        PFence->SetEventOnCompletion(fenceValue, FenceEvent);
        WaitForSingleObject(FenceEvent, INFINITE);
    }
}

bool IsInputMessage(UINT msg)
{
    switch (msg)
    {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_XBUTTONDBLCLK:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_CHAR:
    case WM_IME_COMPOSITION:
    case WM_INPUT:
        return true;
    }
    return false;
}

LRESULT WINAPI hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY || msg == WM_CLOSE)
        Archon::GetConfigManager()->Save(xorstr_("Default"));

    auto HandleInput = [&](bool isMouse, KeyType state, int index, LPARAM lp = 0) -> bool
    {
        bool isRepeat = false;
        if (!isMouse && state == Pressed)
        {
            isRepeat = (lp & (1 << 30)) != 0;
        }

        KeyEvent event = {isMouse, state, index, false, isRepeat};
        Archon::OnKeyEvent(&event);
        return event.cancel;
    };

    switch (msg)
    {
    case WM_KEYDOWN: if (HandleInput(false, Pressed, (int)wParam, lParam)) return true;
        break;
    case WM_KEYUP: if (HandleInput(false, Released, (int)wParam)) return true;
        break;

    case WM_LBUTTONDOWN: if (HandleInput(true, Pressed, 0)) return true;
        break;
    case WM_LBUTTONUP: if (HandleInput(true, Released, 0)) return true;
        break;
    case WM_RBUTTONDOWN: if (HandleInput(true, Pressed, 1)) return true;
        break;
    case WM_RBUTTONUP: if (HandleInput(true, Released, 1)) return true;
        break;
    case WM_MBUTTONDOWN: if (HandleInput(true, Pressed, 2)) return true;
        break;
    case WM_MBUTTONUP: if (HandleInput(true, Released, 2)) return true;
        break;

    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
        {
            int index = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4;
            KeyType state = (msg == WM_XBUTTONDOWN) ? Pressed : Released;
            if (HandleInput(true, state, index)) return true;
        }
        break;
    }

    if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    auto currentUI = Archon::GetCurrentUI();

    if (currentUI == ArchonUI && IsInputMessage(msg))
        return true;

    if (currentUI == ChatUI)
    {
        if ((msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || msg == WM_INPUT)
            return true;
    }

    return CallWindowProc(WndProc, hWnd, msg, wParam, lParam);
}


HRESULT APIENTRY hkPresent(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (g_IsUnloading)
    {
        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    static UINT g_SrvDescSize = 0;
    static D3D12_CPU_DESCRIPTOR_HANDLE g_SrvCpuStart = {};
    static D3D12_GPU_DESCRIPTOR_HANDLE g_SrvGpuStart = {};
    static UINT g_SrvDescCurrentOffset = 1;

    if (!CommandQueue)
        return oPresent(pSwapChain, SyncInterval, Flags);

    if (!Initialized)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&Pd3dDevice)))
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            (void)io;
            io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
            io.IniFilename = nullptr;
            io.LogFilename = nullptr;

            FontManager::Get().Initialize(io);

            ImGui::StyleColorsDark();

            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            NumFrames = sd.BufferCount;
            GameWindow = sd.OutputWindow;
            
            FrameCommandAllocators.resize(NumFrames);
            FrameFenceValues.resize(NumFrames, 0);

            D3D12_DESCRIPTOR_HEAP_DESC srv_desc = {};
            srv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            srv_desc.NumDescriptors = 2048;
            srv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            srv_desc.NodeMask = 0;

            if (FAILED(Pd3dDevice->CreateDescriptorHeap(&srv_desc, IID_PPV_ARGS(&Pd3dSrvDescHeap))))
                return oPresent(pSwapChain, SyncInterval, Flags);

            g_SrvDescSize = Pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            g_SrvCpuStart = Pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
            g_SrvGpuStart = Pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart();

            D3D12_DESCRIPTOR_HEAP_DESC rtv_desc = {};
            rtv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtv_desc.NumDescriptors = NumFrames + 1;
            rtv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            rtv_desc.NodeMask = 1;

            if (FAILED(Pd3dDevice->CreateDescriptorHeap(&rtv_desc, IID_PPV_ARGS(&Pd3dRtvDescHeap))))
                return oPresent(pSwapChain, SyncInterval, Flags);

            for (UINT i = 0; i < NumFrames; i++)
            {
                if (FAILED(Pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&FrameCommandAllocators[i]))))
                    return oPresent(pSwapChain, SyncInterval, Flags);
            }

            if (FAILED(Pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, FrameCommandAllocators[0], NULL, IID_PPV_ARGS(&Pd3dCommandList))) || FAILED(Pd3dCommandList->Close()))
                return oPresent(pSwapChain, SyncInterval, Flags);

            if (FAILED(Pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&PFence))))
                return oPresent(pSwapChain, SyncInterval, Flags);

            FenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (FenceEvent == NULL)
                return oPresent(pSwapChain, SyncInterval, Flags);

            Archon::Initialize();
            ImGui_ImplWin32_Init(GameWindow);

            ID3D12Resource* pBackBuffer = nullptr;
            pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
            DXGI_FORMAT realFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
            if (pBackBuffer)
            {
                realFormat = pBackBuffer->GetDesc().Format;
                pBackBuffer->Release();
            }
            
            Archon::GetArchonGUI()->m_LogoTexture = TextureUtil::LoadImmediate(
                 rawData_logo_Image, 
                 sizeof(rawData_logo_Image),
                 Pd3dDevice,
                 Pd3dSrvDescHeap,
                 CommandQueue,
                 FrameCommandAllocators[0],
                 g_SrvCpuStart,
                 g_SrvGpuStart,
                 g_SrvDescSize,
                 g_SrvDescCurrentOffset
             );

            Shader2DUtil::Get().Init(
                Pd3dDevice,
                realFormat,
                Pd3dSrvDescHeap,
                g_SrvCpuStart,
                g_SrvGpuStart,
                g_SrvDescSize,
                &g_SrvDescCurrentOffset
            );

            D3D12_CPU_DESCRIPTOR_HANDLE maskRtvHandle = Pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
            UINT rtvDescriptorSize = Pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            maskRtvHandle.ptr += (SIZE_T)NumFrames * rtvDescriptorSize;

            Shader2DUtil::Get().InitMesh(
                Pd3dDevice,
                maskRtvHandle,
                rtvDescriptorSize,
                &g_SrvDescCurrentOffset,
                g_SrvCpuStart,
                g_SrvGpuStart,
                g_SrvDescSize
            );

            ImGui_ImplDX12_InitInfo init_info = {};
            init_info.Device = Pd3dDevice;
            init_info.CommandQueue = CommandQueue;
            init_info.NumFramesInFlight = NumFrames;
            init_info.RTVFormat = realFormat;
            init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
            init_info.SrvDescriptorHeap = Pd3dSrvDescHeap;

            init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
            {
                out_cpu_handle->ptr = g_SrvCpuStart.ptr + g_SrvDescCurrentOffset * g_SrvDescSize;
                out_gpu_handle->ptr = g_SrvGpuStart.ptr + g_SrvDescCurrentOffset * g_SrvDescSize;
                g_SrvDescCurrentOffset++;
            };
            init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE)
            {
            };

            ImGui_ImplDX12_Init(&init_info);

            FrameCommandAllocators[0]->Reset();
            Pd3dCommandList->Reset(FrameCommandAllocators[0], nullptr);

            ImGui_ImplDX12_CreateDeviceObjects();

            Pd3dCommandList->Close();
            ID3D12CommandList* ppCommandLists[] = {Pd3dCommandList};
            CommandQueue->ExecuteCommandLists(1, ppCommandLists);

            CommandQueue->Signal(PFence, FenceLastSignaledValue + 1);
            FenceLastSignaledValue++;
            if (PFence->GetCompletedValue() < FenceLastSignaledValue)
            {
                PFence->SetEventOnCompletion(FenceLastSignaledValue, FenceEvent);
                WaitForSingleObject(FenceEvent, INFINITE);
            }

            WndProc = (WNDPROC)SetWindowLongPtr(GameWindow, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
            Initialized = true;
        }
        else
        {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
    }

    FrameIndex = pSwapChain->GetCurrentBackBufferIndex();

    if (FrameFenceValues[FrameIndex] != 0 && PFence->GetCompletedValue() < FrameFenceValues[FrameIndex]) {
        PFence->SetEventOnCompletion(FrameFenceValues[FrameIndex], FenceEvent);
        WaitForSingleObject(FenceEvent, INFINITE);
    }
    FrameCommandAllocators[FrameIndex]->Reset();


    ID3D12Resource* pCurrentBackBuffer = nullptr;
    if (FAILED(pSwapChain->GetBuffer(FrameIndex, IID_PPV_ARGS(&pCurrentBackBuffer))))
        return oPresent(pSwapChain, SyncInterval, Flags);

    SIZE_T rtvDescriptorSize = Pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = Pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += FrameIndex * rtvDescriptorSize;

    Pd3dDevice->CreateRenderTargetView(pCurrentBackBuffer, nullptr, rtvHandle);

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    if (io.DeltaTime <= 0.0f) io.DeltaTime = 1.0f / 60.0f;

    RECT rect;
    GetClientRect(GameWindow, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    Pd3dCommandList->Reset(FrameCommandAllocators[FrameIndex], nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = pCurrentBackBuffer;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    Pd3dCommandList->ResourceBarrier(1, &barrier);

    Pd3dCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    
    DX12DrawContext context = {};
    context.CommandList = Pd3dCommandList;
    context.BackBuffer = pCurrentBackBuffer;
    context.RtvHandle = rtvHandle;
    context.WindowsDrawList = ImGui::GetForegroundDrawList();
    context.BackGroundDrawList = ImGui::GetBackgroundDrawList();
    context.ScreenWidth = (float)width;
    context.ScreenHeight = (float)height;
    context.DeltaTime = io.DeltaTime;

    Shader2DUtil::Get().BeginFrame(Pd3dCommandList, pCurrentBackBuffer, rtvHandle);
    
    Archon::OnDraw(context);

    ImGui::Render();

    ID3D12DescriptorHeap* heaps[] = {Pd3dSrvDescHeap};
    Pd3dCommandList->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Pd3dCommandList);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    Pd3dCommandList->ResourceBarrier(1, &barrier);

    Pd3dCommandList->Close();
    ID3D12CommandList* ppCommandLists[] = {Pd3dCommandList};
    CommandQueue->ExecuteCommandLists(1, ppCommandLists);

    FenceLastSignaledValue++;
    CommandQueue->Signal(PFence, FenceLastSignaledValue);
    FrameFenceValues[FrameIndex] = FenceLastSignaledValue;

    pCurrentBackBuffer->Release();

    return oPresent(pSwapChain, SyncInterval, Flags);
}

void STDMETHODCALLTYPE hkExecuteCommandLists(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists,
                                             ID3D12CommandList* const* ppCommandLists)
{
    if (pCommandQueue->GetDesc().Type == D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        CommandQueue = pCommandQueue;
    }

    return oExecuteCommandLists(pCommandQueue, NumCommandLists, ppCommandLists);
}

bool DX12Hook::Initialize()
{
    VMProtectBeginVirtualization("Initializing DX12Hook");

    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
        xorstr_("DX12 Dummy"), NULL
    };
    RegisterClassEx(&wc);
    HWND hWnd = CreateWindow(wc.lpszClassName, NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance,
                             NULL);

    IDXGIFactory4* pFactory;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&pFactory)))
    {
        DestroyWindow(hWnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return false;
    }

    ID3D12Device* pDummyDevice;
    if (FAILED(D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&pDummyDevice)))
    {
        pFactory->Release();
        DestroyWindow(hWnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC cqDesc = {};
    cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ID3D12CommandQueue* pDummyCommandQueue;
    if (FAILED(pDummyDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&pDummyCommandQueue))))
    {
        pDummyDevice->Release();
        pFactory->Release();
        DestroyWindow(hWnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return false;
    }

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    IDXGISwapChain* pDummySwapChain;
    if (FAILED(pFactory->CreateSwapChain(pDummyCommandQueue, &sd, &pDummySwapChain)))
    {
        pDummyCommandQueue->Release();
        pDummyDevice->Release();
        pFactory->Release();
        DestroyWindow(hWnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return false;
    }

    uintptr_t* pSwapChainVTable = *(uintptr_t**)pDummySwapChain;
    void* pPresentAddress = (void*)pSwapChainVTable[8];

    uintptr_t* pCommandQueueVTable = *(uintptr_t**)pDummyCommandQueue;
    void* pExecuteCommandListsAddress = (void*)pCommandQueueVTable[10];

    pDummySwapChain->Release();
    pDummyCommandQueue->Release();
    pDummyDevice->Release();
    pFactory->Release();
    DestroyWindow(hWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    if (MH_CreateHook(pPresentAddress, &hkPresent, reinterpret_cast<void**>(&oPresent)) != MH_OK)
    {
        Log::Error(xorstr_("CreateHook (Present) failed!"));
        VMProtectEnd();
        return false;
    }

    if (MH_CreateHook(pExecuteCommandListsAddress, &hkExecuteCommandLists,
                      reinterpret_cast<void**>(&oExecuteCommandLists)) != MH_OK)
    {
        Log::Error(xorstr_("CreateHook (ExecuteCommandLists) failed!"));
        VMProtectEnd();
        return false;
    }

    Log::Success(xorstr_("DX12 hooks initialized successfully!"));
    VMProtectEnd();
    return true;
}

void FlushGPU(ID3D12CommandQueue* pQueue, ID3D12Fence* pFence, HANDLE hEvent, UINT64& lastValue) {
    if (!pQueue || !pFence || !hEvent) return;
    lastValue++;
    if (SUCCEEDED(pQueue->Signal(pFence, lastValue))) {
        if (pFence->GetCompletedValue() < lastValue) {
            pFence->SetEventOnCompletion(lastValue, hEvent);
            WaitForSingleObject(hEvent, 2000);
        }
    }
}

void DX12Hook::Shutdown()
{
    g_IsUnloading = true;

    bool windowExists = (GameWindow != nullptr && IsWindow(GameWindow));

    
    if (CommandQueue && PFence && FenceEvent) {
        FlushGPU(CommandQueue, PFence, FenceEvent, FenceLastSignaledValue);
    }
    
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    
    if (WndProc && GameWindow) {
        SetWindowLongPtr(GameWindow, GWLP_WNDPROC, (LONG_PTR)WndProc);
        WndProc = nullptr;
    }
    
    MH_DisableHook(MH_ALL_HOOKS);
    MH_RemoveHook(MH_ALL_HOOKS);
    
    if (Pd3dCommandList) { Pd3dCommandList->Release(); Pd3dCommandList = nullptr; }
    if (Pd3dSrvDescHeap) { Pd3dSrvDescHeap->Release(); Pd3dSrvDescHeap = nullptr; }
    if (Pd3dRtvDescHeap) { Pd3dRtvDescHeap->Release(); Pd3dRtvDescHeap = nullptr; }
    
    for (auto& allocator : FrameCommandAllocators) {
        if (allocator) allocator->Release();
    }
    FrameCommandAllocators.clear();

    if (PFence) { PFence->Release(); PFence = nullptr; }
    if (FenceEvent) { CloseHandle(FenceEvent); FenceEvent = nullptr; }
    if (Pd3dDevice) { Pd3dDevice->Release(); Pd3dDevice = nullptr; }

    MH_Uninitialize();
    
    Log::Success(xorstr_("DX12Hook shutdown successfully."));

    if (!windowExists)
    {
        TerminateProcess(GetCurrentProcess(), 0);
    }
}