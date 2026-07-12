#pragma once
#include "../Module.h"
#include "../../Utils/World/FTickCallbackSystem.h"

class FullBright : public Module
{
public:
    const std::vector<std::string>& GetViewModeNames()
    {
        static const std::vector<std::string> names = {
            xorstr_("BrushWireframe"),     // 0
            xorstr_("Wireframe"),          // 1
            xorstr_("Unlit"),              // 2
            xorstr_("Lit"),                // 3
            xorstr_("DetailLighting"),     // 4
            xorstr_("LightingOnly"),       // 5
            xorstr_("LightComplexity"),    // 6
            xorstr_("IDK(None)"),          // 7 (None)
            xorstr_("ShaderComplexity"),   // 8
            xorstr_("LightmapDensity"),    // 9
            xorstr_("LitLightmapDens"),    // 10
            xorstr_("Reflections"),        // 11
            xorstr_("BufferVis"),          // 12
            xorstr_("Voxel(Disabled)"),    // 13 (Disabled)
            xorstr_("StatLightOverlap"),   // 14
            xorstr_("PlayerCollision"),    // 15
            xorstr_("VisCollision"),       // 16
            xorstr_("Unused(Disabled)"),   // 17 (Disabled)
            xorstr_("MeshLODColor"),       // 18
            xorstr_("QuadOverdraw"),       // 19
            xorstr_("PrimitiveDist"),      // 20
            xorstr_("MeshUVDensity"),      // 21
            xorstr_("Shader&Quads"),       // 22
            xorstr_("HLODColoration"),     // 23
            xorstr_("GroupLODColor"),      // 24
            xorstr_("MatTexScales"),       // 25
            xorstr_("ReqTexRes"),          // 26
            xorstr_("PathTracing"),        // 27
            xorstr_("RayTracingDebug"),    // 28
            xorstr_("Max"),                // 29
            xorstr_("Unknown")             // 255
        };
        return names;
    }
    
    ModeSetting* mode = New<>(ModeSetting(xorstr_("Mode"), 1, GetViewModeNames()));
    
    FullBright() : Module(xorstr_("FullBright"))
    {
    }

    void OnShutdownModule() override
    {
        if (IsEnabled()) OnDisable();
    }

    void OnUpdate() override
    {
        auto LocalPlayer = Instances::GetLocalPlayer();
        
        UGameViewportClient* Viewport = LocalPlayer->ViewportClient;
        
        if (!Viewport) return;

        int32 index = mode->index;
        if (index >= 30) index = 255;
         
        int32* pViewMode = (int32*)((uintptr_t)Viewport + 0xB0);
        
        *pViewMode = index;
    }

    void OnDisable() override
    {
        auto LocalPlayer = Instances::GetLocalPlayer();
        
        UGameViewportClient* Viewport = LocalPlayer->ViewportClient;
        
        if (!Viewport) return;
         
        int32* pViewMode = (int32*)((uintptr_t)Viewport + 0xB0);
        
        *pViewMode = 3;
        
    }
};
