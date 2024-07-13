#include <cstdint>
#include <iostream>

#include "{{type_name}}.h"

namespace {{ns_tpl}}
{
    std::ostream& operator<<(std::ostream& os, {{type_name}} value)
    {
		switch(value)
		{
            {%- for attr in type_info.attrs %}
            case {{type_name}}::{{attr|enum.name}}:
			{
				os << "{{attr|enum.name}}";
				break;
			}
            {%- endfor %}
			default:
			{
				os << "UNKNOWN";
				break;
			}
		}

		return os;
    }
} // namespace {{ns_tpl}}