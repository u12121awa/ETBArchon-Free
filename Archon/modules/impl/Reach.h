#pragma once
#include "../Module.h"

class Reach : public Module
{
public:
    BooleanSetting* interactFix = New<>(BooleanSetting(xorstr_("InteractFix"), true));
    NumberSetting* interactDistance = New<>(NumberSetting(xorstr_("InteractDistance"), 1000.f, 1.0f, 2000.f, xorstr_("%.0f")));
    NumberSetting* smartFindRadius = New<>(NumberSetting(xorstr_("SmartFindRadius"), 65.f, 1.0f, 150.f, xorstr_("%.0f"), [this] { return interactFix->value; }));
    BindSetting* pBind = New<>(BindSetting(xorstr_("KBind"), Bind(-1)));
    
    Reach() : Module(xorstr_("Reach"))
    {
    }
    
    void OnUpdate() override;
    void OnFunctionEvent(FunctionEvent* event) override;
    void OnKeyEvent(KeyEvent* event) override;
    
    void OnShutdownModule() override
    {
       if (IsEnabled()) OnDisable();
    }
    
    void OnDisable() override
    {
        auto PC = Instances::GetPlayerController();
        auto Pawn = Instances::GetPlayerCharacter();
        if (!PC || !Pawn) return;

        Pawn->TraceLength = 200.0f;
    }

};
