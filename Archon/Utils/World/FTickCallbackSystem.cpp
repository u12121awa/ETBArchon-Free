#include "pch.h"

#include "FTickCallbackSystem.h"
    
std::vector<FTickCallbackSystem::FTickCallback> PendingCallbacks;
    
void FTickCallbackSystem::RegisterCallback(FTickCallback Callback)
{
    if (Callback)
    {
        PendingCallbacks.push_back(Callback);
    }
}
    
void FTickCallbackSystem::ExecuteAllCallbacks()
{
    for (auto& Callback : PendingCallbacks)
    {
        try
        {
            Callback();
        }
        catch (...)
        {
            Log::Error("Error executing callback!");
        }
    }
    PendingCallbacks.clear();
}
    
void FTickCallbackSystem::ClearCallbacks()
{
    PendingCallbacks.clear();
}