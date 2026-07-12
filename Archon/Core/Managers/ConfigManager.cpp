#include "pch.h"
#include "ConfigManager.h"

#include <iostream>
#include <algorithm>
#include <fstream>
#include <filesystem> 
#include <errno.h> 

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "Winmm.lib")

#include "ModuleManager.h"

namespace fs = std::filesystem;

std::string CConfigManager::ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    if (from.empty()) return str;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

CConfigManager::CConfigManager()
{
    SetupConfigPaths();
    CreateDirectories();
}

void CConfigManager::SetupConfigPaths()
{
    HARDCODED_BASE_DIR = xorstr_("C:\\ArchonFree");
    CONFIG_SUBDIR = xorstr_("Configs");
    CONFIGS_FOLDER = HARDCODED_BASE_DIR + "\\" + CONFIG_SUBDIR;
}

void CConfigManager::CreateDirectories()
{
    try {
        if (!fs::exists(HARDCODED_BASE_DIR))
            fs::create_directory(HARDCODED_BASE_DIR);
        
        if (!fs::exists(CONFIGS_FOLDER))
            fs::create_directory(CONFIGS_FOLDER);
    }
    catch (const std::exception& e) {
        MessageBeep(MB_ICONHAND);
        Log::Error(std::string(xorstr_("Failed to create directories: ")) + e.what());
    }
}

std::string CConfigManager::GetConfigPath(const std::string& name)
{
    return CONFIGS_FOLDER + "\\" + name + ".ah";
}

void CConfigManager::SerializeModule(const std::shared_ptr<Module>& m, json& modulesArray)
{
    json moduleObject;
    json attribs;

    for (const auto& setting : m->settings)
    {
        if (GroupSetting* groupSetting = dynamic_cast<GroupSetting*>(setting.get()))
        {
            attribs[setting->name] = groupSetting->expand;
        }
        else if (BooleanGroupSetting* booleanGroupSetting = dynamic_cast<BooleanGroupSetting*>(setting.get()))
        {
            attribs[setting->name] = {booleanGroupSetting->value, booleanGroupSetting->expand};
        }
        else if (BooleanSetting* booleanSetting = dynamic_cast<BooleanSetting*>(setting.get()))
        {
            attribs[setting->name] = booleanSetting->value;
        }
        else if (NumberSetting* numberSetting = dynamic_cast<NumberSetting*>(setting.get()))
        {
            attribs[setting->name] = numberSetting->value;
        }
        else if (ModeSetting* modeSetting = dynamic_cast<ModeSetting*>(setting.get()))
        {
            attribs[setting->name] = modeSetting->index;
        }
        else if (StringSetting* stringSetting = dynamic_cast<StringSetting*>(setting.get()))
        {
            std::string encodedText = stringSetting->text;
            encodedText = ReplaceAll(encodedText, " ", "$$");
            attribs[setting->name] = encodedText;
        }
        else if (BindSetting* bindSetting = dynamic_cast<BindSetting*>(setting.get()))
        {
            attribs[setting->name] = { bindSetting->bind.key, bindSetting->bind.mouse, bindSetting->bind.hold };
        }
        else if (ColorSetting* colorSetting = dynamic_cast<ColorSetting*>(setting.get()))
        {
            attribs[setting->name] = { 
                colorSetting->color[0], colorSetting->color[1], colorSetting->color[2], colorSetting->color[3], colorSetting->rainbow
            };
        }
    }
    
    moduleObject[m->name] = attribs;
    modulesArray.push_back(moduleObject);
}

void CConfigManager::DeserializeModule(const json& moduleObject, const std::shared_ptr<Module>& targetModule)
{
    if (!targetModule || moduleObject.is_null() || !moduleObject.contains(targetModule->name)) return;

    const json& mobject = moduleObject.at(targetModule->name);

    for (const auto& setting : targetModule->settings)
    {
        const std::string& name = setting->name;
        if (!mobject.contains(name)) continue;

        try
        {
            if (GroupSetting* groupSetting = dynamic_cast<GroupSetting*>(setting.get()))
            {
                if (mobject.at(name).is_boolean()) groupSetting->expand = mobject.at(name).get<bool>();
            }
            if (BooleanGroupSetting* booleanGroupSetting = dynamic_cast<BooleanGroupSetting*>(setting.get()))
            {
                if (mobject.at(name).is_array() && mobject.at(name).size() >= 2)
                {
                    booleanGroupSetting->value = mobject.at(name)[0].get<bool>();
                    booleanGroupSetting->expand = mobject.at(name)[1].get<bool>();
                }
            }
            else if (BooleanSetting* booleanSetting = dynamic_cast<BooleanSetting*>(setting.get()))
            {
                if (mobject.at(name).is_boolean()) booleanSetting->value = mobject.at(name).get<bool>();
            }
            else if (NumberSetting* numberSetting = dynamic_cast<NumberSetting*>(setting.get()))
            {
                if (mobject.at(name).is_number()) numberSetting->value = mobject.at(name).get<float>();
            }
            else if (ModeSetting* modeSetting = dynamic_cast<ModeSetting*>(setting.get()))
            {
                if (mobject.at(name).is_number_integer()) 
                {
                    int index = mobject.at(name).get<int>();
                    if (index >= 0 && index < modeSetting->modes.size()) modeSetting->index = index;
                }
            }
            else if (StringSetting* stringSetting = dynamic_cast<StringSetting*>(setting.get()))
            {
                if (mobject.at(name).is_string()) 
                {
                    std::string loadedText = mobject.at(name).get<std::string>();
                    loadedText = ReplaceAll(loadedText, "$$", " ");
                    stringSetting->text = loadedText;
                }
            }
            else if (BindSetting* bindSetting = dynamic_cast<BindSetting*>(setting.get()))
            {
                if (mobject.at(name).is_array() && mobject.at(name).size() >= 3)
                {
                    bindSetting->bind.key = mobject.at(name)[0].get<int>();
                    bindSetting->bind.mouse = mobject.at(name)[1].get<bool>();
                    bindSetting->bind.hold = mobject.at(name)[2].get<bool>();
                }
            }
            else if (ColorSetting* colorSetting = dynamic_cast<ColorSetting*>(setting.get()))
            {
                if (mobject.at(name).is_array() && mobject.at(name).size() >= 5)
                {
                    colorSetting->color[0] = mobject.at(name)[0].get<float>();
                    colorSetting->color[1] = mobject.at(name)[1].get<float>();
                    colorSetting->color[2] = mobject.at(name)[2].get<float>();
                    colorSetting->color[3] = mobject.at(name)[3].get<float>();
                    colorSetting->rainbow = mobject.at(name)[4].get<bool>();
                    colorSetting->UpdateHSB(); 
                }
            }
        }
        catch (const std::exception& e)
        {
            MessageBeep(MB_ICONHAND);
            Log::Error(std::string(xorstr_("Setting load error: ")) + e.what());
        }
    }
}

void CConfigManager::Save(const std::string& name)
{
    std::string filePath = GetConfigPath(name);
    std::string tempPath = filePath + ".tmp";
    
    json modulesArray = json::array();
    for (auto& m : ModuleManager::Get().modules)
    {
        SerializeModule(m, modulesArray);
    }
    
    json modulesObj = {{xorstr_("Modules"), modulesArray}}; 
    json rootArray = json::array();
    rootArray.push_back(modulesObj);

    try
    {
        std::ofstream o(tempPath);
        if (!o.is_open()) {
            throw std::runtime_error(xorstr_("Cannot open temp file for writing."));
        }
        
        o << std::setw(4) << rootArray << std::endl;
        o.close(); 
        
        if (fs::exists(filePath)) {
            fs::remove(filePath);
        }
        fs::rename(tempPath, filePath);

        Log::Success(xorstr_("Saved config: ") + filePath, RGBA(128, 0, 128, 255));
    }
    catch (const std::exception& e)
    {
        MessageBeep(MB_ICONHAND);
        Log::Error(std::string(xorstr_("Config Save Failed: ")) + e.what());
        
        if (fs::exists(tempPath)) fs::remove(tempPath);
    }
}

void CConfigManager::Load(const std::string& name)
{
    std::string filePath = GetConfigPath(name);
    if (!fs::exists(filePath))
    {
        return;
    }

    json rootArray;
    try
    {
        std::ifstream i(filePath);
        if (!i.good()) return;
        
        i >> rootArray; 
        
        Log::Success(xorstr_("Loaded config: ") + filePath, RGBA(128, 0, 128, 255));
    }
    catch (json::parse_error& e)
    {
        Log::Error(std::string(xorstr_("Config corrupted (Parse Error): ")) + e.what());
        Log::Error(xorstr_("Resetting to defaults to prevent crash."));
        
        try {
            std::string badFile = filePath + ".bak";
            if (fs::exists(badFile)) fs::remove(badFile);
            fs::rename(filePath, badFile);
            MessageBeep(MB_ICONHAND);
            Log::Error(xorstr_("Bad config moved to .bak file"));
        } catch(...) {}

        return; 
    }
    catch (const std::exception& e)
    {
        MessageBeep(MB_ICONHAND);
        Log::Error(std::string(xorstr_("Config Load Error: ")) + e.what());
        return;
    }

    if (rootArray.is_array() && rootArray.size() > 0 && rootArray[0].contains(xorstr_("Modules")))
    {
        const json& modulesArray = rootArray[0].at(xorstr_("Modules"));

        for (const auto& moduleJson : modulesArray)
        {
            for (auto it = moduleJson.begin(); it != moduleJson.end(); ++it)
            {
                std::string moduleName = it.key();
                
                for (auto& m : ModuleManager::Get().modules)
                {
                    if (m->name == moduleName)
                    {
                        DeserializeModule(moduleJson, m);
                        m->OnInitialize();
                        break;
                    }
                }
            }
        }
    }
}