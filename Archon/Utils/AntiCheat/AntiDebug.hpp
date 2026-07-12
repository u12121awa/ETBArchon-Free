#pragma once
#include <Windows.h>
#include <winternl.h>
#include <intrin.h>
#include <string>
#include "vmaware.hpp"

#pragma warning(disable : 6387)
#pragma warning(disable : 4244)
#pragma warning(disable : 6262)
#pragma warning(disable : 4733)
#pragma warning(disable : 4731)

typedef NTSTATUS(NTAPI* _NtQueryInformationProcess)(HANDLE, ULONG, PVOID, ULONG, PULONG);
typedef NTSTATUS(NTAPI* _NtSetInformationThread)(HANDLE, ULONG, PVOID, ULONG);

namespace AntiDebug {

    enum class SecurityStatus {
        Safe,
        DebuggerPeb,
        RemoteDebugger,
        DebuggerPresent,
        NtQueryInfoProcess,
        CloseHandleException,
        DebugString,
        TimingRdtsc,
        TimingTickCount,
        HardwareRegisters,
        VirtualMachine
    };

    inline void ToLower(unsigned char* input) {
        char* p = (char*)input;
        unsigned long length = (unsigned long)strlen(p);
        for (unsigned long i = 0; i < length; i++) p[i] = tolower(p[i]);
    }

    inline const wchar_t* GetString(int index) {
        std::string value = "";
        switch (index) {
        case 0: value = xorstr_("ntdll.dll"); break;
        case 1: value = xorstr_("NtQueryInformationProcess"); break;
        case 2: value = xorstr_("NtSetInformationThread"); break;
        case 3: value = xorstr_("anti-debugging test."); break;
        }
        static std::wstring ws;
        ws = std::wstring(value.begin(), value.end());
        return ws.c_str();
    }

    inline SecurityStatus CheckPeb() {
#ifdef _WIN64
        PPEB pPeb = (PPEB)__readgsqword(0x60);
        DWORD ntGlobalFlag = *(PDWORD)((PBYTE)pPeb + 0xBC);
#else
        PPEB pPeb = (PPEB)__readfsdword(0x30);
        DWORD ntGlobalFlag = *(PDWORD)((PBYTE)pPeb + 0x68);
#endif
        if (pPeb->BeingDebugged) return SecurityStatus::DebuggerPeb;
        if (ntGlobalFlag & 0x70) return SecurityStatus::DebuggerPeb;
        
        return SecurityStatus::Safe;
    }

    inline SecurityStatus CheckRemoteDebugger() {
        BOOL found = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &found);
        return found ? SecurityStatus::RemoteDebugger : SecurityStatus::Safe;
    }

    inline SecurityStatus CheckDebuggerPresent() {
        return IsDebuggerPresent() ? SecurityStatus::DebuggerPresent : SecurityStatus::Safe;
    }

    inline SecurityStatus CheckNtQueryInfo() {
        DWORD found = FALSE;
        HMODULE hNtdll = LoadLibraryW(GetString(0));
        if (!hNtdll) return SecurityStatus::Safe;

        _NtQueryInformationProcess NtQueryInfo = (_NtQueryInformationProcess)GetProcAddress(hNtdll, xorstr_("NtQueryInformationProcess"));
        if (!NtQueryInfo) return SecurityStatus::Safe;

        NTSTATUS status = NtQueryInfo(GetCurrentProcess(), 0x07, &found, sizeof(DWORD), NULL);
        if (!status && found) return SecurityStatus::NtQueryInfoProcess;

        status = NtQueryInfo(GetCurrentProcess(), 0x1F, &found, sizeof(DWORD), NULL);
        if (!status && !found) return SecurityStatus::NtQueryInfoProcess;

        return SecurityStatus::Safe;
    }

    inline void SetThreadInfo() {
        HMODULE hNtdll = LoadLibraryW(GetString(0));
        if (!hNtdll) return;

        _NtSetInformationThread NtSetThread = (_NtSetInformationThread)GetProcAddress(hNtdll, xorstr_("NtSetInformationThread"));
        if (NtSetThread) NtSetThread(GetCurrentThread(), 0x11, 0, 0);
    }

    inline SecurityStatus CheckCloseHandle() {
        __try { CloseHandle((HANDLE)0xDEADBEEF); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return SecurityStatus::CloseHandleException; }
        return SecurityStatus::Safe;
    }

    inline SecurityStatus CheckDebugString() {
        SetLastError(0);
        OutputDebugStringW(GetString(3));
        return (GetLastError() != 0) ? SecurityStatus::DebugString : SecurityStatus::Safe;
    }

    inline SecurityStatus CheckRdtsc() {
        unsigned __int64 t1 = __rdtsc();
        volatile int sum = 0;
        for (int i = 0; i < 10000; ++i) sum += i;
        unsigned __int64 t2 = __rdtsc();
        return ((t2 - t1) > 0xFFFFFF) ? SecurityStatus::TimingRdtsc : SecurityStatus::Safe;
    }

    inline SecurityStatus CheckTickCount() {
        DWORD64 t1 = GetTickCount64();
        volatile int sum = 0;
        for (int i = 0; i < 10000; ++i) sum += i;
        DWORD64 t2 = GetTickCount64();
        return ((t2 - t1) > 30) ? SecurityStatus::TimingTickCount : SecurityStatus::Safe;
    }

    inline SecurityStatus CheckHardwareRegisters() {
        CONTEXT ctx = { 0 };
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(GetCurrentThread(), &ctx)) {
            if (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3 || ctx.Dr6 || ctx.Dr7)
                return SecurityStatus::HardwareRegisters;
        }
        return SecurityStatus::Safe;
    }

    inline SecurityStatus CheckVirtualMachine() {
        if (VM::detect()) return SecurityStatus::VirtualMachine;
        return SecurityStatus::Safe;
    }

    inline SecurityStatus EvaluateEnvironment() {
        if (CheckVirtualMachine() != SecurityStatus::Safe) return SecurityStatus::VirtualMachine;
        if (CheckPeb() != SecurityStatus::Safe) return SecurityStatus::DebuggerPeb;
        if (CheckRemoteDebugger() != SecurityStatus::Safe) return SecurityStatus::RemoteDebugger;
        if (CheckDebuggerPresent() != SecurityStatus::Safe) return SecurityStatus::DebuggerPresent;
        if (CheckNtQueryInfo() != SecurityStatus::Safe) return SecurityStatus::NtQueryInfoProcess;

        SetThreadInfo();

        if (CheckCloseHandle() != SecurityStatus::Safe) return SecurityStatus::CloseHandleException;
        if (CheckDebugString() != SecurityStatus::Safe) return SecurityStatus::DebugString;
        if (CheckRdtsc() != SecurityStatus::Safe) return SecurityStatus::TimingRdtsc;
        if (CheckTickCount() != SecurityStatus::Safe) return SecurityStatus::TimingTickCount;
        if (CheckHardwareRegisters() != SecurityStatus::Safe) return SecurityStatus::HardwareRegisters;

        return SecurityStatus::Safe;
    }
}