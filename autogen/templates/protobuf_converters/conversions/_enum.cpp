#include "{{path_package}}/utils/PopulateMutex.h"
#include "{{type_name}}.h"

namespace {{ns_tpl}}
{
    bool Convert{{type_name}}::from_protobuf({{path_package}}::{{type_name}}& dest, const {{path_package}}::{{type_name}}& src)
    {
        const auto lock = std::scoped_lock{utils::populateMutex};
        switch (src)
        {
            
            {%- for attr in type_info.attrs %}
            case {{path_package}}::{{type_name}}::{{type_name}}_{{attr|enum.name}}:
            {
                dest = {{path_package}}::{{type_name}}::{{attr|enum.name}};
				break;
			} 
            {%- endfor %}
            default:
            {
			    return false;
			}
        }

		return true;
	}

    bool Convert{{type_name}}::to_protobuf({{path_package}}::{{type_name}}& dest, const {{path_package}}::{{type_name}}& src)
    {
        const auto lock = std::scoped_lock{utils::populateMutex};
        switch (src)
        {
            {%- for attr in type_info.attrs %}
            case {{path_package}}::{{type_name}}::{{attr|enum.name}}:
            {
                dest = {{path_package}}::{{type_name}}::{{type_name}}_{{attr|enum.name}};
				break;
			} 
            {%- endfor %}
            default:
            {
			    return false;
			}
        }

		return true;
    }
} // namespace {{ns_tpl}}