#pragma once


#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include <Windows.h> 
#include <direct.h> 

#include "../../modules/Module.h"
#include "../../Utils/Nlohmann/json.hpp"

using json = nlohmann::ordered_json;

class CConfigManager
{
private:
    std::string HARDCODED_BASE_DIR;
    std::string CONFIG_SUBDIR;
    
    std::string CONFIGS_FOLDER;
    
    void CreateDirectories();
    void SetupConfigPaths();
    
    void SerializeModule(const std::shared_ptr<Module>& m, json& modulesArray);
    void DeserializeModule(const json& moduleObject, const std::shared_ptr<Module>& targetModule);

public:
    CConfigManager();
    ~CConfigManager() = default;
    
    void Load(const std::string& name);
    void Save(const std::string& name);
    
    std::string GetConfigPath(const std::string& name);
    
    std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);
};

