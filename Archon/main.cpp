#include "pch.h"
#include <thread>    // 引入线程库
#include <chrono>    // 引入时间库

#include "Archon.h"
#include "Core/Instance/Instances.hpp"
#include "Core/Config.h"
#include "Core/Memory.h"
#include "Core/Module/features.h"
#include "Hook/Hooks.h"
#include "Utils/AntiCheat/AntiDebug.hpp"

void SecurityWatchdog()
{
    while (!Config::Settings::Unload)
    {
        AntiDebug::SecurityStatus status = AntiDebug::EvaluateEnvironment();
        
        if (status != AntiDebug::SecurityStatus::Safe)
        {
            int errorCode = static_cast<int>(status);
            
            std::string msg = "Security violation detected!\nError Code: " + std::to_string(errorCode) + "\n\nProcess will terminate in 8 seconds.";
            
            std::thread([]() {
                std::this_thread::sleep_for(std::chrono::seconds(8));
                ExitProcess(0);
            }).detach();
            
            MessageBoxA(NULL, msg.c_str(), "Archon Security", MB_OK | MB_ICONERROR | MB_TOPMOST);
            
            ExitProcess(0); 
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
DWORD WINAPI Thread(HMODULE hModule)
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    
    //std::thread(SecurityWatchdog).detach();

    if (!Hooks::Initialize())
    {
        if (f) fclose(f);
        FreeConsole();
        FreeLibraryAndExitThread(hModule, 1);
        return 1;
    }
    
    while (!Config::Settings::Unload)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Log::Info(xorstr_("Ejecting..."));
    Hooks::Shutdown();

    if (f) fclose(f);
    FreeConsole();
    
    FreeLibraryAndExitThread(hModule, 0);
    
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Thread, hModule, 0, nullptr);
    }
    return TRUE;
}