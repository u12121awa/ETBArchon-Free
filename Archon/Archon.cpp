#include "pch.h"

#include "Archon.h"

#include "Core/Managers/ActorsFindManager.h"
#include "Core/Managers/ConfigManager.h"
#include "Core/Managers/DynamicIslandManager.h"
#include "Core/Managers/ModuleManager.h"
#include "Core/Managers/RotationManager.h"
#include "Core/Managers/SoundManager.h"
#include "Core/Managers/WorldManager.h"
#include "Utils/World/Events.h"
#include "Utils/World/FTickCallbackSystem.h"

GWorldManager* WorldManager;
MCChatSystem* ChatSystem;
ArchonGUI* ArchonGui;
CConfigManager* ConfigManager;
RotationManager* RotateManager;
DynamicIslandManager* DynamicIslandManager;

void Archon::Initialize()
{
    VMProtectBeginUltra("Initializing All");
    SoundManager::Get().Init();
    SoundManager::Get().LoadAllSound();
    WorldManager = new GWorldManager();
    RotateManager = new RotationManager();
    ModuleManager::Get().Init();
    ConfigManager = new CConfigManager();
    ConfigManager->Load(xorstr_("Default"));
    DynamicIslandManager = new class DynamicIslandManager();
    ChatSystem = new MCChatSystem();
    ArchonGui = new ArchonGUI();
    VMProtectEnd();
}

void Archon::OnUpdate()
{
    VMProtectBeginMutation("Archon::OnUpdate");
    Instances::Update();
    if (RotateManager) RotateManager->OnUpdate();
    if (WorldManager) WorldManager->OnUpdate();
    ModuleManager::Get().OnUpdate();
    if (ChatSystem) ChatSystem->OnUpdate();
    FTickCallbackSystem::ExecuteAllCallbacks();
    std::shared_ptr<ClickGUI> clickGUI = ModuleManager::Get().GetModule<ClickGUI>();
    if (clickGUI) clickGUI->OnUpdate();
    VMProtectEnd();
}


void Archon::OnFunctionEvent(FunctionEvent* event)
{
    VMProtectBeginMutation("Archon::OnFunctionEvent");
    if (WorldManager) WorldManager->OnFunctionEvent(event);
    ModuleManager::Get().OnFunctionEvent(event);
    if (ChatSystem) ChatSystem->OnFunctionEvent(event);
    if (RotateManager) RotateManager->OnFunctionEvent(event);
    VMProtectEnd();
}


void Archon::OnKeyEvent(KeyEvent* event)
{
    VMProtectBeginMutation("Archon::OnKeyEvent");
    ModuleManager::Get().OnKeyEvent(event);
    ModuleRenderer::OnKeyEvent(event);
    WidgetRenderer::OnKeyEvent(event);
    if (event->cancel)
    {
        VMProtectEnd();
        return;
    }
    
    if (ChatSystem)
    {
        ChatSystem->OnKeyEvent(event);
        if (event->cancel)
        {
            VMProtectEnd();
            return;
        }
    }
    
    if (event->type == Pressed && event->key == VK_RETURN && GetCurrentUI() == None)
    {
        auto chatSystem = ModuleManager::Get().GetModule<class ChatSystem>();
        
        if (chatSystem && chatSystem->IsEnabled() && !ChatSystem->IsOpen())
        {
            ChatSystem->Toggle();
            event->cancel = true; 
        }
    }

    if (event->type == Pressed && event->key == VK_ESCAPE)
    {
        if (ArchonGui->IsOpen())
        {
            ArchonGui->Toggle();
            event->cancel = true;
        }

        if (ChatSystem->IsOpen())
        {
            ChatSystem->SetEnabled(false);
            event->cancel = true;
        }
    }
    VMProtectEnd();
}

void Archon::OnDraw(DX12DrawContext context)
{
    Instances::Update(false);
    ModuleManager::Get().OnRender2D(context);
    DynamicIslandManager->OnRender2D(context);
    ChatSystem->Draw();
    ArchonGui->DrawMainGUI(context);
}

CurrentUI Archon::GetCurrentUI()
{
    if (ArchonGui && ArchonGui->IsOpen()) return ArchonUI;
    if (ChatSystem && ChatSystem->IsOpen()) return ChatUI;
    return None;
}


ArchonGUI* Archon::GetArchonGUI()
{
    return ArchonGui;
}

MCChatSystem* Archon::GetMCChatSystem()
{
    return ChatSystem;
}

CConfigManager* Archon::GetConfigManager()
{
    return ConfigManager;
}

RotationManager* Archon::GetRotationManager()
{
    return RotateManager;
}

GWorldManager* Archon::GetWorldManager()
{
    return WorldManager;
}

class DynamicIslandManager* Archon::GetDynamicIslandManager()
{
    return DynamicIslandManager;
}

