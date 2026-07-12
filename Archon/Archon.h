#pragma once

#include "Core/Managers/ConfigManager.h"
#include "Core/Managers/DynamicIslandManager.h"
#include "Core/Managers/RotationManager.h"
#include "Core/Managers/WorldManager.h"
#include "UI/ArchonGUI.h"
#include "UI/MCChatSystem.h"
#include "Utils/Render/DX12DrawContext.h"

enum CurrentUI { ArchonUI, ChatUI , None};

namespace Archon
{
    void Initialize();
    void OnUpdate();
    void OnFunctionEvent(FunctionEvent* event);
    void OnKeyEvent(KeyEvent* event);
    void OnDraw(DX12DrawContext context);
    CurrentUI GetCurrentUI();
    ArchonGUI* GetArchonGUI();
    MCChatSystem* GetMCChatSystem();
    CConfigManager* GetConfigManager();
    RotationManager* GetRotationManager();
    GWorldManager* GetWorldManager();
    DynamicIslandManager* GetDynamicIslandManager();
}
