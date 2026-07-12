#include "pch.h"
#include "InputUtils.h"

#include "../Archon.h"

bool InputUtils::IsGuiActive()
{
    return Archon::GetCurrentUI() != None;
}

