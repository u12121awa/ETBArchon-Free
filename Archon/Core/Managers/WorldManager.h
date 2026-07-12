#pragma once
#include "../../Utils/World/Events.h"

class GWorldManager
{
private:
    float TICK_TIMER = 1.0f;
    int Ping = -1;
    std::string PlayerName;

public:
    void OnFunctionEvent(FunctionEvent* event);
    void OnUpdate();
    void SetSpeed(float NewSpeed);
    float GetSpeed() const { return TICK_TIMER; }
    int GetPing() const { return Ping; }
    std::string GetPlayerName() const { return PlayerName; }
    void Reset();
private:

};
