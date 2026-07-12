#pragma once
#include <windows.h>

class InputUtils
{
private:
    static inline bool KeyStates[256] = { false };

    static bool IsGameWindowActive()
    {
        HWND hForeground = GetForegroundWindow();
        if (!hForeground) return false;

        DWORD foregroundPid;
        GetWindowThreadProcessId(hForeground, &foregroundPid);

        return (foregroundPid == GetCurrentProcessId());
    }

    static bool IsGuiActive();

public:
    
    static bool IsKeyDown(int vKey, bool uiCheck = false)
    {
        if (uiCheck && IsGuiActive()) return false;
        if (!IsGameWindowActive()) return false;
        
        return (GetAsyncKeyState(vKey) & 0x8000) != 0;
    }

    static bool IsKeyPressed(int vKey, bool uiCheck = false)
    {
        if (uiCheck && IsGuiActive()) return false;
        if (!IsGameWindowActive()) return false;
        
        bool isDown = (GetAsyncKeyState(vKey) & 0x8000) != 0;
        
        if (isDown && !KeyStates[vKey])
        {
            KeyStates[vKey] = true; 
            return true;
        }
        if (!isDown)
        {
            KeyStates[vKey] = false;
        }

        return false;
    }
};