#include "pch.h"
#include "WorldUtils.h"

#include "../../Archon.h"

float WorldUtils::GetPing()
{
    return Archon::GetWorldManager()->GetPing();
}
