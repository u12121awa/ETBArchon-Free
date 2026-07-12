#pragma once

#include <vector>
#include <functional>

namespace FTickCallbackSystem
{
    using FTickCallback = std::function<void()>;
    
    void RegisterCallback(FTickCallback Callback);
    void ExecuteAllCallbacks();
    void ClearCallbacks();
}