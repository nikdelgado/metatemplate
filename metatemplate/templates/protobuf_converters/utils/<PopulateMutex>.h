#pragma once

#include <mutex>

namespace {{ns_tpl}}
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    static inline std::recursive_mutex populateMutex{};
} // namespace {{ns_tpl}}