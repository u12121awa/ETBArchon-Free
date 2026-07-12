#pragma once
#include <memory>
#include <vector>

#include "../../modules/Module.h"
#include "../../Utils/Render/DX12DrawContext.h"


class ModuleManager
{
public:
    static ModuleManager& Get()
    {
        static ModuleManager instance;
        return instance;
    }

    std::vector<std::shared_ptr<Module>> modules;
    bool bIsShutdown = true;

    void Init();
    void Shutdown();
    void OnRender2D(DX12DrawContext context);
    void OnFunctionEvent(FunctionEvent* event);
    void OnRespawnChange();
    void OnWorldChange();
    void OnKeyEvent(KeyEvent* event);
    void OnUpdate();
    std::vector<std::shared_ptr<Module>> GetModulesByCategory(const std::string& category);

    template <typename T>
    std::shared_ptr<T> GetModule()
    {
        if (bIsShutdown) return nullptr;
        for (auto& mod : modules)
        {
            auto casted = std::dynamic_pointer_cast<T>(mod);
            if (casted) return casted;
        }
        return nullptr;
    }
};
