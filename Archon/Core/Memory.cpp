#include "pch.h";
#include "Memory.h"

namespace Memory 
{
    __declspec(noinline) uintptr_t PatternScan(uintptr_t moduleAdress, const char* signature)
    {
        VMProtectBeginMutation("PatternScan_Fast");

        static auto patternToByte = [](const char* pattern)
        {
            auto       bytes = std::vector<int>{};
            const auto start = const_cast<char*>(pattern);
            const auto end = const_cast<char*>(pattern) + strlen(pattern);

            for (auto current = start; current < end; ++current)
            {
                if (*current == '?')
                {
                    ++current;
                    bytes.push_back(-1);
                }
                else { bytes.push_back(strtoul(current, &current, 16)); }
            }
            return bytes;
        };

        const auto dosHeader = (PIMAGE_DOS_HEADER)moduleAdress;
        const auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)moduleAdress + dosHeader->e_lfanew);

        const auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
        auto       patternBytes = patternToByte(signature);
        const auto scanBytes = reinterpret_cast<std::uint8_t*>(moduleAdress);

        const auto s = patternBytes.size();
        const auto d = patternBytes.data();
        
        for (auto i = 0ul; i < sizeOfImage - s; ++i)
        {
            bool found = true;
            for (auto j = 0ul; j < s; ++j)
            {
                if (scanBytes[i + j] != d[j] && d[j] != -1)
                {
                    found = false;
                    break;
                }
            }
            if (found) { 
                uintptr_t result = reinterpret_cast<uintptr_t>(&scanBytes[i]);
                VMProtectEnd(); 
                return result;
            }
        }

        VMProtectEnd();
        return NULL;
    }
    
    __declspec(noinline) uintptr_t FindPattern(const char* Signature)
    {
        VMProtectBeginMutation("Scan_Pattern_Fast");

        static auto PatternToByte = [](const char* Pattern)
        {
            auto Bytes = std::vector<int>{};
            const auto Start = const_cast<char*>(Pattern);
            const auto End = const_cast<char*>(Pattern) + strlen(Pattern);

            for (auto Current = Start; Current < End; ++Current)
            {
                if (*Current == '?')
                {
                    ++Current;
                    if (*Current == '?')
                        ++Current;
                    Bytes.push_back(-1);
                }
                else
                {
                    Bytes.push_back(strtoul(Current, &Current, 16));
                }
            }
            return Bytes;
        };
        
        const auto DosHeader = (PIMAGE_DOS_HEADER)GetModuleHandle(NULL);
        const auto NtHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)DosHeader + DosHeader->e_lfanew);

        const auto SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
        auto PatternBytes = PatternToByte(Signature);
        const auto ScanBytes = reinterpret_cast<std::uint8_t*>(DosHeader);

        const auto s = PatternBytes.size();
        const auto d = PatternBytes.data();
        
        for (auto i = 0ul; i < SizeOfImage - s; ++i)
        {
            bool Found = true;
            for (auto j = 0ul; j < s; ++j)
            {
                if (ScanBytes[i + j] != d[j] && d[j] != -1)
                {
                    Found = false;
                    break;
                }
            }
            if (Found)
            {
                uintptr_t result = (uintptr_t)&ScanBytes[i];
                VMProtectEnd(); 
                return result;
            }
        }

        VMProtectEnd(); 
        return NULL;
    }
}