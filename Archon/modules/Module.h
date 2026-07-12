#pragma once

#include <iostream>

#include "../Utils/ClientData.h"
#include "../Utils/Render/DX12DrawContext.h"
#include "../Utils/World/Events.h"
#include "settings/Settings.h"
#include "../Core/Instance/Instances.hpp"
#include "../Core/Managers/NotificationManager.h"
#include "../Core/Managers/SoundManager.h"

class Module
{
public:
    std::string name;
    std::string displayInfo;
    std::string category;
    std::shared_ptr<BooleanSetting> enabled;
    std::shared_ptr<BooleanSetting> expanded;
    std::shared_ptr<BindSetting> bind;
    std::shared_ptr<NumberSetting> height;
  
    std::vector<std::shared_ptr<Setting>> settings;

    FloatAnimation heightAnim;
    FloatAnimation toggleAnim;

    Module(std::string name, std::string category = xorstr_("Modules")) : name(name), category(category)
    {
        enabled = RegisterSetting(std::make_shared<BooleanSetting>(xorstr_("Enabled"), false, []{ return false; }));
        expanded = RegisterSetting(std::make_shared<BooleanSetting>(xorstr_("Expanded"), false, []{ return false; }));
        bind = RegisterSetting(std::make_shared<BindSetting>(xorstr_("Bind"), Bind(-1), []{ return false; }));
        height = RegisterSetting(std::make_shared<NumberSetting>(xorstr_("Height"), 40.f, 0, 0, xorstr_("%.0f"), []{ return false; }));
    }

    virtual ~Module() = default;

    void OnInitialize()
    {
        heightAnim.Init(expanded ? height->value : 40.0f);
        OnInitializeModule();
    }

    void OnShutdown()
    {
        OnShutdownModule();
    }

    void Toggle()
    {
        enabled->value = !enabled->value;
        if (enabled->value) Enable();
        else Disable();
    }

    void Enable()
    {
        enabled->value = true;
        OnEnable();
        Log::Info(xorstr_("Module enabled ") + name);
        
        if (name != xorstr_("ClickGUI") && name != xorstr_("UnLoad"))
        {
            SoundManager::Get().PlayModule(enabled->value);
            NotificationManager::AddModuleToggle(this);
        }
    }

    void Disable()
    {
        enabled->value = false;
        OnDisable();
        Log::Info(xorstr_("Module disabled ") + name);
        
        if (name != xorstr_("ClickGUI") && name != xorstr_("UnLoad"))
        {
            SoundManager::Get().PlayModule(enabled->value);
            NotificationManager::AddModuleToggle(this);
        }
    }

    bool IsEnabled()
    {
        return enabled->value;
    }

    bool IsDisabled()
    {
        return !enabled->value;
    }

    bool CanExpanded()
    {
        for (auto& s : settings)
        {
            if (s->IsVisible()) return true;
        }
        return false;
    }
    
    template<typename T>
    std::shared_ptr<T> RegisterSetting(std::shared_ptr<T> s)
    {
        std::shared_ptr<Setting> setting = s;
        
        if (setting) {
            setting->module = this->name;
            settings.push_back(setting);
        }
        return s;
    }
    
    template <typename T>
    T* New(T setting, Setting* group = nullptr)
    {
        std::shared_ptr<T> ptr = std::make_shared<T>(setting);

        if (group) ptr->SetGroup(group);
        
        this->RegisterSetting(ptr);
        
        return ptr.get();
    }

    virtual void OnEnable()
    {
    }

    virtual void OnDisable()
    {
    }

    virtual void OnUpdate()
    {
    }

    virtual void OnKeyEvent(KeyEvent* event)
    {
    }

    virtual void OnFunctionEvent(FunctionEvent* event)
    {
    }

    virtual void OnRespawnChange()
    {
    }
    
    virtual void OnWorldChange()
    {
    }

    virtual void OnRender2D(DX12DrawContext context)
    {
    }

    virtual void OnInitializeModule()
    {
    }

    virtual void OnShutdownModule()
    {
        
    }
};

