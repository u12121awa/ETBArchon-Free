#pragma once
#include "../imgui/imgui.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx12.h"
#include "../imgui/imgui_impl_win32.h"
#include "../Utils/Minhook/MinHook.h"


namespace Hooks
{
    bool onInitSDK();
    bool Initialize();
    void Shutdown();
}
