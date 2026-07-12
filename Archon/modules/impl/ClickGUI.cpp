#include "pch.h"
#include "ClickGUI.h"

#include "../../Archon.h"


void ClickGUI::OnEnable()
{
    Archon::GetArchonGUI()->Toggle();
    this->Disable();
}

