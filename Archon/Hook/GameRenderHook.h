#pragma once

typedef void (*PostRender_t)(UObject* pObject, UCanvas* pCanvas);

namespace GameRenderHook
{
    bool Initialize();
    void MainRender(UObject* object, UCanvas* Canvas);
    void Shutdown();
}
