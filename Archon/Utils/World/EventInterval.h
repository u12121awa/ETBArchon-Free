#pragma once

#include <cstddef>

namespace EventInterval
{
    void OnEvent(const char* name, std::size_t historyCount = 120);
}
