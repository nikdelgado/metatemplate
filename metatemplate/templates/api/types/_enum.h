#pragma once

#include <cstdint>
#include <iostream>

namespace {{ns_tpl}}
{
    /**
	 * {{type_info.help|clean_docstring}}
	 */
    enum class {{type_name}} : {{type_info|enum.class_base or 'int'}}
    {
        {%- for attr in type_info.attrs %}
        {{attr|enum.name}} = {{attr|enum.value or loop.index}}{{"," if not loop.last}}
        {%- endfor %}
    };

	/**
	 * @brief Output stream operator for the {{type_name}}
	 *
	 * @param os: the output stream
	 * @param value: the {{type_name}} enum
	 * @returns std::ostream
	 */
    std::ostream& operator<<(std::ostream& os, {{type_name}} value);
} // namespace {{ns_tpl}}