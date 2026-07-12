#pragma once
#include <Windows.h>
#include <cstdint>

namespace Memory
{
    template <typename T>
    inline T Read(DWORD64 address)
    {
        if (!address || IsBadReadPtr((void*)address, sizeof(T)))
        {
            return T{};
        }
        __try
        {
            return *(T*)address;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return T{};
        }
    }

    template <typename T>
    inline bool Write(DWORD64 address, T value)
    {
        if (!address || IsBadWritePtr((void*)address, sizeof(T)))
        {
            return false;
        }
        __try
        {
            *(T*)address = value;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
    
    inline void memcpy_(void* _Dst, void const* _Src, size_t _Size)
    {
        auto csrc = (char*)_Src;
        auto cdest = (char*)_Dst;

        for (int i = 0; i < _Size; i++)
        {
            cdest[i] = csrc[i];
        }
    }

    template <typename T4>
    inline bool WriteMemW(uintptr_t ptr, T4 const& Value)
    {
        if (ptr < 0x10000000 || ptr > 0x7fffffffffff) return false;
        DWORD d, ds;
        int val2;
        VirtualProtect((LPVOID)ptr, sizeof(Value), PAGE_EXECUTE_READWRITE, &d);
        *reinterpret_cast<T4*>(ptr) = Value;
        VirtualProtect((LPVOID)ptr, sizeof(Value), d, &ds);
        return true;
    }

    inline uintptr_t ChangePointer(uintptr_t ptr, int Index, uintptr_t Value)
    {
        uintptr_t ptrAddr = ptr + (0x8 * Index);
        uintptr_t* blabla = (uintptr_t*)ptrAddr;
        uintptr_t AddressBefore = *blabla;

        WriteMemW(ptrAddr, Value);

        return AddressBefore;
    }

    uintptr_t FindPattern(const char* Signature);
    uintptr_t PatternScan(uintptr_t moduleAdress, const char* signature);
    inline DWORD64 ReadPtr(DWORD64 address) { return Read<DWORD64>(address); }
    inline float ReadFloat(DWORD64 address) { return Read<float>(address); }
    inline int ReadInt(DWORD64 address) { return Read<int>(address); }
    inline bool WriteFloat(DWORD64 address, float value) { return Write<float>(address, value); }
}
