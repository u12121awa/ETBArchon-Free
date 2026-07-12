#pragma once

#include <psapi.h>
#include <windows.h>
#include <DbgHelp.h>
#include <vector>
#include <string>
#include <mmsystem.h>

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "Winmm.lib")
#include "../Hook/FunctionHook.h"
#include <vector>
#include <string>
#include <set>
#include <sstream>

namespace Utils
{
    __declspec(noinline) inline std::string Hex(uintptr_t value)
    {
        std::stringstream ss;
        ss << std::hex << value;
        return ss.str();
    }

    __declspec(noinline) inline void CopyToClipboard(const std::string& text)
    {
        if (OpenClipboard(NULL))
        {
            EmptyClipboard();
            HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
            if (hClipboardData)
            {
                char* pchData = (char*)GlobalLock(hClipboardData);
                if (pchData)
                {
                    memcpy(pchData, text.c_str(), text.size() + 1);
                    GlobalUnlock(hClipboardData);
                    SetClipboardData(CF_TEXT, hClipboardData);
                }
            }
            CloseClipboard();
        }
    }

    __declspec(noinline) inline void PrintStackTrace(PCONTEXT Context, std::string& clipboardBuffer)
    {
        auto LogAndCapture = [&](const char* msg)
        {
            Log::Error(msg);
            clipboardBuffer += msg;
            clipboardBuffer += "\n";
        };

        LogAndCapture(xorstr_("---------------------- [CALL STACK] ----------------------"));

        HANDLE hProcess = GetCurrentProcess();
        HANDLE hThread = GetCurrentThread();

        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

        if (!SymInitialize(hProcess, NULL, TRUE))
        {
            LogAndCapture(xorstr_("Failed to initialize symbols."));
            return;
        }

        STACKFRAME64 stackFrame;
        memset(&stackFrame, 0, sizeof(stackFrame));

        stackFrame.AddrPC.Offset = Context->Rip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = Context->Rbp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = Context->Rsp;
        stackFrame.AddrStack.Mode = AddrModeFlat;

        char buf[1024];
        int frameNum = 0;

        while (StackWalk64(
            IMAGE_FILE_MACHINE_AMD64,
            hProcess,
            hThread,
            &stackFrame,
            Context,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL
        ))
        {
            if (stackFrame.AddrPC.Offset == 0) break;

            char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolBuffer;
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 displacement = 0;
            std::string funcName = xorstr_("UnknownFunction");

            if (SymFromAddr(hProcess, stackFrame.AddrPC.Offset, &displacement, pSymbol))
            {
                funcName = pSymbol->Name;
            }

            IMAGEHLP_LINE64 lineInfo;
            memset(&lineInfo, 0, sizeof(IMAGEHLP_LINE64));
            lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

            DWORD displacementLine = 0;
            std::string fileInfo = "";

            if (SymGetLineFromAddr64(hProcess, stackFrame.AddrPC.Offset, &displacementLine, &lineInfo))
            {
                fileInfo = std::string(xorstr_(" [")) + lineInfo.FileName + xorstr_(":") + std::to_string(lineInfo.LineNumber) + xorstr_("]");
            }

            sprintf_s(buf, xorstr_("[%d] 0x%llX | %s%s"),
                      frameNum,
                      stackFrame.AddrPC.Offset,
                      funcName.c_str(),
                      fileInfo.c_str());

            LogAndCapture(buf);

            frameNum++;

            if (frameNum > 50) break;
        }

        LogAndCapture(xorstr_("========================================================================"));
        LogAndCapture(" ");

        SymCleanup(hProcess);
    }

    __declspec(noinline) inline void GetAddressInfo(PVOID Address, char* OutBuffer, size_t BufferSize)
    {
        HMODULE hModule = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)Address, &hModule))
        {
            char ModName[MAX_PATH];
            GetModuleBaseNameA(GetCurrentProcess(), hModule, ModName, MAX_PATH);

            uintptr_t Offset = (uintptr_t)Address - (uintptr_t)hModule;

            sprintf_s(OutBuffer, BufferSize, "%s + 0x%llX", ModName, Offset);
        }
        else
        {
            sprintf_s(OutBuffer, BufferSize, xorstr_("Unknown Module (0x%p)"), Address);
        }
    }

    __declspec(noinline) inline long __stdcall PrintCrashInfo(_EXCEPTION_POINTERS* ExceptionInfo, const char* ContextName)
    {
        static ULONGLONG LastSoundTime = 0;
        ULONGLONG CurrentTime = GetTickCount64();

        if (CurrentTime - LastSoundTime > 2000)
        {
            MessageBeep(MB_ICONHAND);
            LastSoundTime = CurrentTime;
        }

        std::string fullLog;
        auto LogAndCapture = [&](const char* msg)
        {
            Log::Error(msg);
            fullLog += msg;
            fullLog += "\n";
        };

        DWORD Code = ExceptionInfo->ExceptionRecord->ExceptionCode;
        PVOID Address = ExceptionInfo->ExceptionRecord->ExceptionAddress;
        PCONTEXT Context = ExceptionInfo->ContextRecord;

        if (Code != EXCEPTION_ACCESS_VIOLATION && Code != EXCEPTION_INT_DIVIDE_BY_ZERO)
            return EXCEPTION_EXECUTE_HANDLER;

        LogAndCapture(" ");
        LogAndCapture(xorstr_("======================== [CRASH DETECTED] ========================"));

        char buf[512];

        sprintf_s(buf, xorstr_("Context : %s"), ContextName ? ContextName : xorstr_("Unknown"));
        LogAndCapture(buf);

        sprintf_s(buf, xorstr_("Code    : 0x%X (%s)"), Code,
                  (Code == EXCEPTION_ACCESS_VIOLATION)
                      ? xorstr_("Access Violation")
                      : (Code == EXCEPTION_INT_DIVIDE_BY_ZERO)
                      ? xorstr_("Divide By Zero")
                      : xorstr_("Other"));
        LogAndCapture(buf);

        if (Code == EXCEPTION_ACCESS_VIOLATION)
        {
            ULONG_PTR ViolationType = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
            ULONG_PTR TargetAddr = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];

            const char* typeStr = (ViolationType == 0) ? xorstr_("READ") : (ViolationType == 1) ? xorstr_("WRITE") : xorstr_("EXECUTE");
            sprintf_s(buf, xorstr_("Error   : Attempted to %s address 0x%p"), typeStr, (void*)TargetAddr);
            LogAndCapture(buf);

            if (TargetAddr < 0x1000) LogAndCapture(xorstr_("Analysis: NULL Pointer Dereference (Empty Object)!"));
            else if (TargetAddr == 0x3f800000) LogAndCapture(xorstr_("Analysis: Bad SDK Struct (Float read as Pointer)!"));
            else if (TargetAddr == 0xFFFFFFFFFFFFFFFF) LogAndCapture(xorstr_("Analysis: Uninitialized/Bad Handle (-1)!"));
        }

        char locationInfo[256];
        GetAddressInfo(Address, locationInfo, sizeof(locationInfo));
        sprintf_s(buf, xorstr_("Location: %s"), locationInfo);
        LogAndCapture(buf);

        LogAndCapture(xorstr_("---------------------- Registers (x64) ----------------------"));
        sprintf_s(buf, xorstr_("RAX: %016llX  RBX: %016llX  RCX: %016llX"), Context->Rax, Context->Rbx, Context->Rcx);
        LogAndCapture(buf);
        sprintf_s(buf, xorstr_("RDX: %016llX  RSI: %016llX  RDI: %016llX"), Context->Rdx, Context->Rsi, Context->Rdi);
        LogAndCapture(buf);
        sprintf_s(buf, xorstr_("R8 : %016llX  R9 : %016llX  R10: %016llX"), Context->R8, Context->R9, Context->R10);
        LogAndCapture(buf);
        sprintf_s(buf, xorstr_("R11: %016llX  RSP: %016llX  RBP: %016llX"), Context->R11, Context->Rsp, Context->Rbp);
        LogAndCapture(buf);

        PrintStackTrace(ExceptionInfo->ContextRecord, fullLog);

        CopyToClipboard(fullLog);
        Log::Error(xorstr_("[System] Crash log copied to clipboard."));

        return EXCEPTION_EXECUTE_HANDLER;
    }

    __declspec(noinline) inline long __stdcall PrintFunctionCrashInfo(_EXCEPTION_POINTERS* ExceptionInfo, const char* FunctionName)
    {
        static ULONGLONG LastSoundTime = 0;
        ULONGLONG CurrentTime = GetTickCount64();

        if (CurrentTime - LastSoundTime > 2000)
        {
            MessageBeep(MB_ICONHAND);
            LastSoundTime = CurrentTime;
        }

        std::string fullLog;
        auto LogAndCapture = [&](const char* msg)
        {
            Log::Error(msg);
            fullLog += msg;
            fullLog += "\n";
        };

        DWORD Code = ExceptionInfo->ExceptionRecord->ExceptionCode;
        PVOID Address = ExceptionInfo->ExceptionRecord->ExceptionAddress;
        PCONTEXT Context = ExceptionInfo->ContextRecord;

        LogAndCapture(" ");
        LogAndCapture(xorstr_("======================== [CRASH DETECTED] ========================"));

        char buf[512];

        sprintf_s(buf, xorstr_("Context : %s"), FunctionName ? FunctionName : xorstr_("Unknown"));
        LogAndCapture(buf);

        sprintf_s(buf, xorstr_("Code    : 0x%X (%s)"), Code,
                  (Code == EXCEPTION_ACCESS_VIOLATION)
                      ? xorstr_("Access Violation")
                      : (Code == EXCEPTION_INT_DIVIDE_BY_ZERO)
                      ? xorstr_("Divide By Zero")
                      : xorstr_("Other"));
        LogAndCapture(buf);

        if (Code == EXCEPTION_ACCESS_VIOLATION)
        {
            ULONG_PTR ViolationType = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
            ULONG_PTR TargetAddr = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];

            const char* typeStr = (ViolationType == 0) ? xorstr_("READ") : (ViolationType == 1) ? xorstr_("WRITE") : xorstr_("EXECUTE");

            sprintf_s(buf, xorstr_("Error   : Attempted to %s address 0x%p"), typeStr, (void*)TargetAddr);
            LogAndCapture(buf);

            if (TargetAddr < 0x1000) LogAndCapture(xorstr_("Analysis: NULL Pointer Dereference!"));
            else if (TargetAddr == 0x3f800000) LogAndCapture(xorstr_("Analysis: Bad SDK Struct (Float read as Pointer)!"));
            else if (TargetAddr == 0xFFFFFFFFFFFFFFFF) LogAndCapture(xorstr_("Analysis: Uninitialized/Bad Handle (-1)!"));
        }

        char locationInfo[256];
        GetAddressInfo(Address, locationInfo, sizeof(locationInfo));
        sprintf_s(buf, xorstr_("Location: %s"), locationInfo);
        LogAndCapture(buf);

        LogAndCapture(xorstr_("---------------------- Registers (x64) ----------------------"));

        sprintf_s(buf, xorstr_("RAX: %016llX  RBX: %016llX  RCX: %016llX"), Context->Rax, Context->Rbx, Context->Rcx);
        LogAndCapture(buf);

        sprintf_s(buf, xorstr_("RDX: %016llX  RSI: %016llX  RDI: %016llX"), Context->Rdx, Context->Rsi, Context->Rdi);
        LogAndCapture(buf);

        sprintf_s(buf, xorstr_("R8 : %016llX  R9 : %016llX  R10: %016llX"), Context->R8, Context->R9, Context->R10);
        LogAndCapture(buf);

        sprintf_s(buf, xorstr_("R11: %016llX  RSP: %016llX  RBP: %016llX"), Context->R11, Context->Rsp, Context->Rbp);
        LogAndCapture(buf);

        PrintStackTrace(ExceptionInfo->ContextRecord, fullLog);

        CopyToClipboard(fullLog);
        Log::Error(xorstr_("[System] Crash log copied to clipboard."));

        return EXCEPTION_EXECUTE_HANDLER;
    }

    __declspec(noinline) inline void SafeFunctionCall(
        ProcessEvent_t OriginalFn,
        UObject* Context,
        UFunction* Function,
        void* Parms,
        const char* FuncName
    )
    {
        __try
        {
            OriginalFn(Context, Function, Parms);
        }
        __except (PrintFunctionCrashInfo(GetExceptionInformation(), FuncName))
        {
        }
    }
#define M_PI 3.14159265358979323846f
    __declspec(noinline) inline float DegToRad(float deg) { return deg * (M_PI / 180.f); }
    
    __declspec(noinline) inline FVector CustomRotateVector(FRotator Rot, FVector V)
    {
        float SP = sin(DegToRad(Rot.Pitch));
        float CP = cos(DegToRad(Rot.Pitch));
        float SY = sin(DegToRad(Rot.Yaw));
        float CY = cos(DegToRad(Rot.Yaw));
        float SR = sin(DegToRad(Rot.Roll));
        float CR = cos(DegToRad(Rot.Roll));
        
        FVector X_Axis;
        X_Axis.X = CP * CY;
        X_Axis.Y = CP * SY;
        X_Axis.Z = SP;

        FVector Y_Axis;
        Y_Axis.X = SR * SP * CY - CR * SY;
        Y_Axis.Y = SR * SP * SY + CR * CY;
        Y_Axis.Z = -SR * CP;

        FVector Z_Axis;
        Z_Axis.X = -(CR * SP * CY + SR * SY);
        Z_Axis.Y = CY * SR - CR * SP * SY;
        Z_Axis.Z = CR * CP;
        
        FVector Result;
        Result.X = (V.X * X_Axis.X) + (V.Y * Y_Axis.X) + (V.Z * Z_Axis.X);
        Result.Y = (V.X * X_Axis.Y) + (V.Y * Y_Axis.Y) + (V.Z * Z_Axis.Y);
        Result.Z = (V.X * X_Axis.Z) + (V.Y * Y_Axis.Z) + (V.Z * Z_Axis.Z);

        return Result;
    }
    
    __declspec(noinline) inline bool GetActor2DBox(ACharacter* Char, APlayerController* PC, FVector4& OutRect)
    {
        if (!Char || !PC) return false;

        UCapsuleComponent* Cap = Char->CapsuleComponent;
        if (!Cap) return false;

        float Radius = Cap->GetScaledCapsuleRadius();
        float HalfHeight = Cap->GetScaledCapsuleHalfHeight();

  

        FVector Origin = Cap->K2_GetComponentLocation();

        FRotator Rot = Cap->K2_GetComponentRotation();

        const FVector BoxPoints[] = {
            FVector(Radius, Radius, HalfHeight),
            FVector(Radius, -Radius, HalfHeight),
            FVector(-Radius, Radius, HalfHeight),
            FVector(-Radius, -Radius, HalfHeight),
            FVector(Radius, Radius, -HalfHeight),
            FVector(Radius, -Radius, -HalfHeight),
            FVector(-Radius, Radius, -HalfHeight),
            FVector(-Radius, -Radius, -HalfHeight)
        };

        float MinX = FLT_MAX, MinY = FLT_MAX;
        float MaxX = -FLT_MAX, MaxY = -FLT_MAX;
        bool bAnyOnScreen = false;

        for (int i = 0; i < 8; i++)
        {
            FVector WorldPoint = Origin + CustomRotateVector(Rot, BoxPoints[i]);

            FVector2D ScreenPos;
            if (PC->ProjectWorldLocationToScreen(WorldPoint, &ScreenPos, true))
            {
                if (ScreenPos.X < MinX) MinX = ScreenPos.X;
                if (ScreenPos.Y < MinY) MinY = ScreenPos.Y;
                if (ScreenPos.X > MaxX) MaxX = ScreenPos.X;
                if (ScreenPos.Y > MaxY) MaxY = ScreenPos.Y;
                bAnyOnScreen = true;
            }
        }

        if (!bAnyOnScreen) return false;

        OutRect.X = MinX;
        OutRect.Y = MinY;
        OutRect.Z = MaxX;
        OutRect.W = MaxY;

        return true;
    }

    __declspec(noinline) inline std::string CleanClassName(const std::string& ClassName)
    {
        std::string name = ClassName;

        if (name.rfind(xorstr_("BP_"), 0) == 0)
        {
            name = name.substr(3);

            if (name.size() > 2 && name.compare(name.size() - 2, 2, xorstr_("_C")) == 0)
            {
                name.resize(name.size() - 2);
            }
        }
        else
        {
            const std::string suffix = xorstr_("_BP_C");
            if (name.size() > suffix.size())
            {
                size_t pos = name.rfind(suffix);
                if (pos != std::string::npos && pos + suffix.size() == name.size())
                {
                    name.resize(pos);
                }
            }
        }

        return name;
    }
    
    __declspec(noinline) inline float FormatFloatValue(float value, const char* format)
    {
        char buffer[64];
        
        std::snprintf(buffer, sizeof(buffer), format, value);
        
        return std::strtof(buffer, nullptr);
    }
    __declspec(noinline) inline DWORD GetMemoryProtection(LPVOID address)
    {
        MEMORY_BASIC_INFORMATION memInfo;
        VirtualQuery(address, &memInfo, sizeof(memInfo));
        return memInfo.Protect;
    }

    __declspec(noinline) inline bool ShouldUsePointer64(void* ptrF)
    {
        uintptr_t ptr = (uintptr_t)ptrF;
        if (ptr == 0x0 || ptr < 0x10000000 || ptr > 0x7fffffffffff || GetMemoryProtection(reinterpret_cast<LPVOID>(ptr))
            == PAGE_NOACCESS) return false;
        return true;
    }

    __declspec(noinline) inline std::string ReadWideStringFromAddress(FString fName, size_t maxLength)
    {
        if (!ShouldUsePointer64(fName.CStr()))
            return "";

        const wchar_t* ptr = fName.CStr();

        if (!ptr)
        {
            return "";
        }


        std::wstring wstr;
        try
        {
            for (size_t i = 0; i < maxLength; ++i)
            {
                wchar_t wc = ptr[i];
                if (wc == L'\0')
                    break;
                wstr += wc;
            }

            if (wstr.empty()) return "";

            int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (bufferSize <= 0)
            {
                return "";
            }

            std::string result(bufferSize, 0);
            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], bufferSize, nullptr, nullptr);

            if (bufferSize > 1)
                result.resize(bufferSize - 1);
            else
                result.clear();

            return result;
        }
        catch (...)
        {
            return "";
        }
    }

    __declspec(noinline) inline std::wstring SanitizeWString(std::wstring Input)
    {
        std::wstring output = L"";

        int Sizecc = Input.size();

        for (size_t i = 0; i < 14; i++)
        {
            if (i >= Sizecc) break;

            output += Input[i];
        }

        return output;
    }
    
    __declspec(noinline) inline std::string WstringToUtf8(const std::wstring& wstr)
    {
        if (wstr.empty())
            return std::string();

        int size_needed = WideCharToMultiByte(
            CP_UTF8,
            0,
            wstr.data(),
            (int)wstr.size(),
            nullptr,
            0,
            nullptr,
            nullptr
        );

        std::string result(size_needed, 0);
        WideCharToMultiByte(
            CP_UTF8,
            0,
            wstr.data(),
            (int)wstr.size(),
            &result[0],
            size_needed,
            nullptr,
            nullptr
        );

        return result;
    }

    __declspec(noinline) inline std::string wstringToString(const std::wstring& wstr)
    {
        std::string str;
        for (wchar_t c : wstr)
        {
            str += static_cast<char>(c);
        }
        return str;
    }

    __declspec(noinline) inline std::wstring stringToWideString(const std::string& str)
    {
        std::wstring wstr(str.length(), L' ');
        std::copy(str.begin(), str.end(), wstr.begin());
        return wstr;
    }

    __declspec(noinline) inline std::wstring Utf8ToWstring(const std::string& str)
    {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    __declspec(noinline) inline FString CharToFString(const char* str)
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
        if (len == 0) return FString();
        wchar_t* wstr = new wchar_t[len];
        MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, len);
        FString fstr(wstr);
        delete[] wstr;
        return fstr;
    }

    /*
    inline bool IsValidSteamID(uint64_t val)
    {
        return val > 76561190000000000ULL;
    }

    inline std::string GetSteamID(ABPCharacter_Demo_C* Target)
    {
        if (!Target || !Target->PlayerState) return xorstr_("NPlayerState");

        uintptr_t BaseAddr = (uintptr_t)&Target->PlayerState->UniqueId;

        std::vector Offsets = {0x08, 0x10};

        for (int offset : Offsets)
        {
            uintptr_t* PtrLocation = (uintptr_t*)(BaseAddr + offset);
            uintptr_t DeepPtr = *PtrLocation;

            if (DeepPtr < 0x10000 || IsBadReadPtr((void*)DeepPtr, 8)) continue;

            for (int i = 0; i < 128; i += 4)
            {
                uint64_t* Val = (uint64_t*)(DeepPtr + i);

                if (IsBadReadPtr(Val, 8)) break;

                if (IsValidSteamID(*Val))
                {
                    return std::to_string(*Val);
                }
            }
        }

        for (int i = 0; i < 64; i += 8)
        {
            uint64_t* Val = (uint64_t*)(BaseAddr + i);
            if (IsValidSteamID(*Val)) return std::to_string(*Val);
        }

        return "";
    }
    
    inline std::string GetSteamIDDebug(ABPCharacter_Demo_C* Target)
    {
        std::cout << "\n----------- [SteamID Debug Start] -----------" << std::endl;

        if (!Target)
        {
            std::cout << "[Error] Target is nullptr." << std::endl;
            return "Invalid Target";
        }
        if (!Target->PlayerState)
        {
            std::cout << "[Error] PlayerState is nullptr (Likely AI/Bot)." << std::endl;
            return "NPlayerState";
        }

        // 1. Get Base Address of UniqueId
        uintptr_t BaseAddr = (uintptr_t)&Target->PlayerState->UniqueId;
        std::cout << "[Info] UniqueId Base Address: 0x" << std::hex << BaseAddr << std::dec << std::endl;

        // We check offset 0x08 and 0x10 because the pointer might shift
        std::vector<int> Offsets = {0x08, 0x10};

        for (int offset : Offsets)
        {
            uintptr_t* PtrLocation = (uintptr_t*)(BaseAddr + offset);

            // Safety check: Is the pointer location itself readable?
            if (IsBadReadPtr(PtrLocation, 8))
            {
                std::cout << "[Skip] Memory at offset +0x" << std::hex << offset << " is unreadable." << std::endl;
                continue;
            }

            uintptr_t DeepPtr = *PtrLocation;

            std::cout << "[Scan] Checking Offset +0x" << std::hex << offset
                << " -> Pointer Value (DeepPtr): 0x" << DeepPtr << std::dec;

            // Check if the pointer points to valid memory
            if (DeepPtr < 0x10000 || IsBadReadPtr((void*)DeepPtr, 8))
            {
                std::cout << " (Invalid/Null/Unreadable)" << std::endl;
                continue;
            }
            std::cout << " (Valid Pointer, scanning content...)" << std::endl;

            // 2. Deep Scan: Check the memory inside the object pointed to
            for (int i = 0; i < 128; i += 4)
            {
                uint64_t* ValPtr = (uint64_t*)(DeepPtr + i);

                // Safety break
                if (IsBadReadPtr(ValPtr, 8))
                {
                    std::cout << "    -> [Stop] Hit unreadable memory at InnerOffset +0x" << std::hex << i << std::endl;
                    break;
                }

                uint64_t Val = *ValPtr;

                // Check if we found the ID
                if (IsValidSteamID(Val))
                {
                    std::cout << "    >>> [SUCCESS] FOUND IT! <<<" << std::endl;
                    std::cout << "    Path: UniqueId + 0x" << std::hex << offset << " -> Points to memory + 0x" << i <<
                        std::endl;
                    std::cout << "    SteamID (Dec): " << std::dec << Val << std::endl;

                    return std::to_string(Val);
                }
            }
            std::cout << "    -> Finished scanning this pointer, no ID found." << std::endl;
        }

        // 3. Fallback: Scan the struct itself (Base Address)
        std::cout << "[Info] Pointer scan failed, scanning struct base..." << std::endl;
        for (int i = 0; i < 64; i += 8)
        {
            uint64_t* ValPtr = (uint64_t*)(BaseAddr + i);
            if (IsBadReadPtr(ValPtr, 8)) continue;

            uint64_t Val = *ValPtr;
            if (IsValidSteamID(Val))
            {
                std::cout << "    >>> [SUCCESS] Found inside struct base! <<<" << std::endl;
                std::cout << "    Offset: +0x" << std::hex << i << std::endl;
                return std::to_string(Val);
            }
        }

        std::cout << "[Result] SteamID not found." << std::endl;
        return "";
    }
    
    inline std::string GetHostSteamID(UWorld* World)
    {
        if (!World || !World->NetDriver || !World->NetDriver->ServerConnection) return "";

        uintptr_t ConnectionAddr = (uintptr_t)World->NetDriver->ServerConnection;

        uintptr_t StartAddr = ConnectionAddr + 0xA0;
        uintptr_t EndAddr = StartAddr + 0x200;

        for (uintptr_t ptr = StartAddr; ptr < EndAddr; ptr += 8)
        {
            FString* PossibleStr = (FString*)ptr;


            std::string str = PossibleStr->ToString();

            size_t found = str.find(xorstr_("steam."));
            if (found != std::string::npos)
            {
                std::string id = str.substr(found + 6);
                size_t colon = id.find(xorstr_(":"));
                if (colon != std::string::npos) id = id.substr(0, colon);
                return id;
            }
        }

        return "";
    }
    */
}
