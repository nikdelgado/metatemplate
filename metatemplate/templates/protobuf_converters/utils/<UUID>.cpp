#include "{{path_package}}/utils/PopulateMutex.h"
#include "UUID.h"
#include <string>
#include <array>

namespace {{ns_package}}::conversions
{
    bool ConvertUuid::from_protobuf(std::array<std::uint8_t, 16>& dest, const c::types::UUID& src)
    {
        const auto lock = std::scoped_lock{utils::populateMutex};
        auto tempStr = src.value();
        std::copy_n(tempStr.begin(), std::min(tempStr.size(), dest.size()), dest.begin());
		return true;
	}

    bool ConvertUuid::to_protobuf(::types::UUID& dest, const std::array<std::uint8_t, 16>& src)
    {
        const auto lock = std::scoped_lock{utils::populateMutex};
        std::string tempStr{};
        std::transform(src.begin(), src.end(), std::back_inserter(tempStr), [](const auto ch) {return static_cast<char>(ch);});
        dest.set_value(std::move(tempStr));
		return true;
	}
} // namespace {{ns_package}}::conversions