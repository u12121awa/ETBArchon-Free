#include "pch.h"

#include "SoundManager.h"
#include <iostream>

#include "ModuleManager.h"
#include "../../modules/impl/SoundFX.h"
#include "../../Utils/RawData/rawData_sound_disabled.h"
#include "../../Utils/RawData/rawData_sound_enabled.h"

#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

bool SoundManager::Init()
{
    if (m_IsInit) return true;

    ma_result result;

    result = ma_engine_init(NULL, &m_Engine);
    if (result != MA_SUCCESS)
    {
        Log::Error(xorstr_("SoundManager: Failed to initialize audio engine."));
        return false;
    }

    m_IsInit = true;
    return true;
}

bool SoundManager::LoadAllSound()
{
    bool b1 = LoadSoundMem(xorstr_("ModuleEnabled"), rawData_sound_enabled, rawData_sound_enabled_len);
    bool b2 = LoadSoundMem(xorstr_("ModuleDisabled"), rawData_sound_disabled, rawData_sound_disabled_len);
    return b1 && b2;
}

bool SoundManager::LoadSoundMem(const std::string& name, const void* pData, size_t size)
{
    if (!m_IsInit) return false;
    if (m_Sounds.find(name) != m_Sounds.end()) return true;

    if (!pData || size == 0)
        return false;

    SoundEntry* entry = new SoundEntry();

    ma_decoder_config decoderConfig = ma_decoder_config_init_default();

    ma_result result = ma_decoder_init_memory(
        pData,
        size,
        &decoderConfig,
        &entry->decoder
    );

    if (result != MA_SUCCESS)
    {
        delete entry;
        return false;
    }

    result = ma_sound_init_from_data_source(
        &m_Engine,
        &entry->decoder,
        MA_SOUND_FLAG_DECODE,
        NULL,
        &entry->sound
    );

    if (result != MA_SUCCESS)
    {
        ma_decoder_uninit(&entry->decoder);
        delete entry;
        return false;
    }

    m_Sounds[name] = entry;
    return true;
}



void SoundManager::Play(const std::string& name, float volume)
{
    if (!m_IsInit) return;

    auto it = m_Sounds.find(name);
    if (it == m_Sounds.end()) return;

    SoundEntry* entry = it->second;

    ma_sound_set_volume(&entry->sound, volume);

    ma_sound_stop(&entry->sound);
    ma_sound_seek_to_pcm_frame(&entry->sound, 0);
    ma_sound_start(&entry->sound);
}


static bool ReadRegStringW(HKEY root, const wchar_t* subKey, const wchar_t* valueName, std::wstring& out)
{
    out.clear();

    HKEY hKey = nullptr;
    if (RegOpenKeyExW(root, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return false;

    DWORD type = 0;
    DWORD bytes = 0;
    LONG r = RegQueryValueExW(hKey, valueName, nullptr, &type, nullptr, &bytes);
    if (r != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || bytes < sizeof(wchar_t))
    {
        RegCloseKey(hKey);
        return false;
    }

    std::wstring buf(bytes / sizeof(wchar_t), L'\0');
    r = RegQueryValueExW(hKey, valueName, nullptr, &type, (LPBYTE)buf.data(), &bytes);
    RegCloseKey(hKey);
    if (r != ERROR_SUCCESS) return false;
    
    while (!buf.empty() && buf.back() == L'\0') buf.pop_back();

    if (type == REG_EXPAND_SZ)
    {
        wchar_t expanded[4096];
        DWORD n = ExpandEnvironmentStringsW(buf.c_str(), expanded, (DWORD)_countof(expanded));
        if (n > 0 && n < _countof(expanded)) out = expanded;
        else out = buf;
    }
    else
    {
        out = buf;
    }

    return !out.empty();
}

static void PlaySystemEventOrWav(const wchar_t* eventName, const wchar_t* regEventKey)
{
    if (PlaySoundW(eventName, NULL, SND_ASYNC | SND_NODEFAULT))
        return;
    
    std::wstring wavPath;
    if (ReadRegStringW(HKEY_CURRENT_USER, regEventKey, L".Current", wavPath))
    {
       
    }
    
    if (!ReadRegStringW(HKEY_CURRENT_USER, regEventKey, nullptr, wavPath))
    {
        MessageBeep(MB_OK);
        return;
    }

    if (!wavPath.empty())
        PlaySoundW(wavPath.c_str(), NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
    else
        MessageBeep(MB_OK);
}

void PlayUsbInsertSound()
{
    PlaySystemEventOrWav(
        L"DeviceConnect",
        L"AppEvents\\Schemes\\Apps\\.Default\\DeviceConnect\\.Current"
    );
}

void PlayUsbRemoveSound()
{
    PlaySystemEventOrWav(
        L"DeviceDisconnect",
        L"AppEvents\\Schemes\\Apps\\.Default\\DeviceDisconnect\\.Current"
    );
}

void SoundManager::PlayModule(bool enabled)
{
    auto soundFX = ModuleManager::Get().GetModule<SoundFX>();
    if (!soundFX || soundFX->IsDisabled()) return;

    float volume = soundFX->volume->value;
    try
    {
        switch (soundFX->enabledMode->index)
        {
        case 1:
            {
                if (enabled) Play(xorstr_("ModuleEnabled"), volume);
                else Play(xorstr_("ModuleDisabled"), volume);
                break;
            }
        case 2:
            {
                if (enabled) PlayUsbInsertSound();
                else PlayUsbRemoveSound();
                break;
            }
        }
    }
    catch (...)
    {
        Log::Error(xorstr_("SoundManager::PlayModule crashed!"));
    }
}

void SoundManager::Shutdown()
{
    if (!m_IsInit) return;

    for (auto& pair : m_Sounds)
    {
        SoundEntry* entry = pair.second;
        ma_sound_uninit(&entry->sound);
        ma_decoder_uninit(&entry->decoder);
        delete entry;
    }

    m_Sounds.clear();

    ma_engine_uninit(&m_Engine);
    m_IsInit = false;
}

