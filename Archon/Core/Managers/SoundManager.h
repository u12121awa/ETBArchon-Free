#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "../../Utils/MiniAudio/miniaudio.h"

struct SoundEntry
{
    ma_sound sound;
    ma_decoder decoder;
};


class SoundManager
{
private:
    ma_engine m_Engine;
    bool m_IsInit = false;
    
    std::unordered_map<std::string, SoundEntry*> m_Sounds;


public:
    static SoundManager& Get()
    {
        static SoundManager instance;
        return instance;
    }
    
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
    SoundManager() = default;

    bool Init();
    bool LoadAllSound();
    
    bool LoadSoundMem(const std::string& name, const void* pData, size_t size);
    
    void Play(const std::string& name, float volume = 1.0f);
    void PlayModule(bool enabled);
    void Shutdown();
};