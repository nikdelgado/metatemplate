#include "{{path_package}}/utils/PopulateMutex.h"
#include "{{type_name}}.h"

namespace {{ns_tpl}}
{
	bool Convert{{type_name}}::from_protobuf({{ns_package}}::{{type_name}}& dest, const {{ns_package}}::{{type_name}}& src)
	{
        const auto lock = std::scoped_lock{utils::populateMutex};
		dest = src.value();
		return true;
	}

    bool Convert{{type_name}}::to_protobuf({{ns_package}}::{{type_name}}& dest, const {{ns_package}}::{{type_name}}& src)
    {
        const auto lock = std::scoped_lock{utils::populateMutex};
        dest.set_value(src);
		return true;
	}
	
} // namespace {{ns_tpl}}