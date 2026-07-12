#include "pch.h"
#include "ModuleManager.h"

#include "../../Archon.h"
#include "../../modules/impl/AntiAim.h"
#include "../../modules/impl/AntiCrash.h"
#include "../../modules/impl/ArrayList.h"
#include "../../modules/impl/AutoConnect.h"
#include "../../modules/impl/BHop.h"
#include "../../modules/impl/BugGod.h"
#include "../../modules/impl/CameraManager.h"
#include "../../modules/impl/ChatSystem.h"
#include "../../modules/impl/DynamicIsland.h"
#include "../../modules/impl/ESP.h"
#include "../../modules/impl/FastUse.h"
#include "../../modules/impl/Flight.h"
#include "../../modules/impl/FreeCam.h"
#include "../../modules/impl/ItemCleaner.h"
#include "../../modules/impl/ItemSpawner.h"
#include "../../modules/impl/LongJump.h"
#include "../../modules/impl/NameTags.h"
#include "../../modules/impl/FullBright.h"
#include "../../modules/impl/KillAura.h"
#include "../../modules/impl/NameChanger.h"
#include "../../modules/impl/NoRender.h"
#include "../../modules/impl/PlayerManager.h"
#include "../../modules/impl/Reach.h"
#include "../../modules/impl/ServerLagger.h"
#include "../../modules/impl/SkinBlinker.h"
#include "../../modules/impl/SoundFX.h"
#include "../../modules/impl/Spam.h"
#include "../../modules/impl/SpawnTest.h"
#include "../../modules/impl/Speed.h"
#include "../../modules/impl/Test.h"
#include "../../modules/impl/Timer.h"
#include "../../modules/impl/UnLoad.h"

void ModuleManager::Init()
{
    VMProtectBeginUltra("Initializing Modules");
    if (!modules.empty()) return;

    modules.push_back(std::make_shared<KillAura>());
    modules.push_back(std::make_shared<AntiAim>());
    modules.push_back(std::make_shared<BHop>());
    modules.push_back(std::make_shared<Speed>());
    modules.push_back(std::make_shared<LongJump>());
    modules.push_back(std::make_shared<Flight>());
    modules.push_back(std::make_shared<BugGod>());
    modules.push_back(std::make_shared<SkinBlinker>());
    modules.push_back(std::make_shared<NameTags>());
    modules.push_back(std::make_shared<ESP>());
    modules.push_back(std::make_shared<NoRender>());
    modules.push_back(std::make_shared<FullBright>());
    modules.push_back(std::make_shared<CameraManager>());
    modules.push_back(std::make_shared<PlayerManager>());
    modules.push_back(std::make_shared<FreeCam>());
    modules.push_back(std::make_shared<Reach>());
    modules.push_back(std::make_shared<Timer>());
    modules.push_back(std::make_shared<Test>());
    modules.push_back(std::make_shared<SpawnTest>());
    modules.push_back(std::make_shared<Spam>());
    modules.push_back(std::make_shared<NameChanger>());
    modules.push_back(std::make_shared<ItemSpawner>());
    modules.push_back(std::make_shared<ItemCleaner>());
    modules.push_back(std::make_shared<AntiCrash>());
    modules.push_back(std::make_shared<ServerLagger>());
    modules.push_back(std::make_shared<AutoConnect>());
    modules.push_back(std::make_shared<ChatSystem>());
    modules.push_back(std::make_shared<DynamicIsland>());
    modules.push_back(std::make_shared<ArrayList>());
    modules.push_back(std::make_shared<ClickGUI>());
    modules.push_back(std::make_shared<SoundFX>());
    modules.push_back(std::make_shared<UnLoad>());
    
    bIsShutdown = false;
    VMProtectEnd();
}

void ModuleManager::Shutdown()
{
    bIsShutdown = true;
    
    for (auto& module : modules)
    {
        if (module) module->OnShutdown();
    }
}

void ModuleManager::OnRender2D(DX12DrawContext context)
{
    if (bIsShutdown) return;
    
    for (auto& module : modules)
    {
        if (module && module->IsEnabled())
        {
            module->OnRender2D(context);
        }
    }
}
void ModuleManager::OnFunctionEvent(FunctionEvent* event)
{
    if (bIsShutdown) return;
    
    for (auto& module : modules)
    {
        if (module && module->IsEnabled())
        {
            module->OnFunctionEvent(event);
        }
    }
}

void ModuleManager::OnRespawnChange()
{
    if (bIsShutdown) return;
    for (auto& module : modules)
    {
        if (module && module->IsEnabled())
        {
            module->OnRespawnChange();
        }
    }
}

void ModuleManager::OnWorldChange()
{
    if (bIsShutdown) return;
    for (auto& module : modules)
    {
        if (module && module->IsEnabled())
        {
            module->OnWorldChange();
        }
    }
}



void ModuleManager::OnKeyEvent(KeyEvent* event)
{
    if (bIsShutdown) return;

    for (auto& module : modules)
    {
        if (module && 
            event->mouse == module->bind->bind.mouse && 
            event->key == module->bind->bind.key && 
            Archon::GetCurrentUI() == None)
        {
            event->cancel = true;

            if (event->type == Pressed)
            {
                if (module->bind->bind.hold)
                {
                    if (module->IsDisabled()) 
                    {
                        module->Enable();
                    }
                }
                else 
                {
                    if (!event->repeat)
                    {
                        module->Toggle();
                    }
                }
            }
            else if (event->type == Released)
            {
                if (module->bind->bind.hold && module->IsEnabled())
                {
                    module->Disable();
                }
            }
        }
        else if (module->IsEnabled())
        {
            module->OnKeyEvent(event);
        }
    }
}

void ModuleManager::OnUpdate()
{
    if (bIsShutdown) return;
    for (auto module : modules)
    {
        if (!module || module->IsDisabled()) continue;
        module->OnUpdate();
    }
}

std::vector<std::shared_ptr<Module>> ModuleManager::GetModulesByCategory(const std::string& category)
{
    std::vector<std::shared_ptr<Module>> result;
    
    
    for (auto& m : modules)
    {
        if (m && m->category == category) result.push_back(m);
    }
    return result;
}
