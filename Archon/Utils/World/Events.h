#pragma once
#include "../../Hook/FunctionHook.h"
#include "../../modules/settings/Settings.h"

struct KeyEvent
{
    bool mouse;
    KeyType type;
    int key;
    bool cancel;
    bool repeat; 
};

struct FunctionEvent
{
    UObject* context;
    UFunction* function;
    void* parms;
    std::string functionName;
    ProcessEvent_t processEvent;
    bool cancel;

    void CallBack(void* pParms)
    {
        if (context && function && parms && processEvent)
        {
            processEvent(context, function, pParms ? pParms :parms);
        }
    }
};