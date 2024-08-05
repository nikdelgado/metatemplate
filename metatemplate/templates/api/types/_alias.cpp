#include <stdexcept>

{%- if false and type_info is alias.has_restriction('pattern') %}
#include <boost/regex.hpp>
{%- endif %}

{%- if type_info is alias.is_float %}
#include {{"utils/EssentiallyEqual.h" | util_ns.incl}}
{%- endif %}


#include {{"utils/Stream.h" | util_ns.incl}}
#include {{"byte_stream/ByteStream.h" | util_ns.incl}}

#include "{{type_name}}.h"

{%- set restriction_map = {
	'min_inclusive': '{var} < {restrict}',
	'min_exclusive': '{var} <= {restrict}',
	'max_inclusive': '{var} > {restrict}',
	'max_exclusive': '{var} >= {restrict}',
	'length': '{var}.length() != {restrict}',
	'min_length': '{var}.length() < {restrict}',
	'max_length': '{var}.length() > {restrict}',
    
} %}
{# TODO: enable patterm matching...copy into restriction map above
had boost build error at some point - also need to remove "false and"
from the include condition above for boost/regex.hpp
'pattern': '!boost::regex_search({var}, boost::regex({restrict}))',
#}

namespace {{ns_tpl}}
{

	{{type_name}}::const_ref_type {{type_name}}::operator*() const noexcept
	{
		return value_;
	}

	{{type_name}}::const_ref_type {{type_name}}::getValue() const noexcept
	{
		return value_;
	}

	void {{type_name}}::setValue({{type_name}}::const_ref_type val)
	{

		value_ = {{type_name}}::checkValue(val);
	}

	{{type_name}}::operator {{type_name}}::alias_type() const
	{
        return getValue();
	}

	void {{type_name}}::toByteStream(byte_stream::OByteStream& bs) const
	{
		bs << value_;
	}

	void {{type_name}}::fromByteStream(byte_stream::IByteStream& bs)
	{
		{{type_info|alias.primitive}} value{};
		bs >> value;
		setValue(value);
	}

	{{type_name}}::const_ref_type {{type_name}}::checkValue({{type_name}}::const_ref_type val)
	{
		{%- for field_name, f_string in restriction_map.items() %}
		{%-   if type_info is alias.has_restriction(field_name) %}
		if ({{f_string.format(var="val",restrict=field_name.upper())}}) {
			throw std::invalid_argument(
				"{{type_name}}::value failed {{field_name}} test"
			);
		}
		{%-   endif %}
		{%- endfor %}

		return val;
	}

    /**
	 * @brief Output stream operator for {{type_name}}
	 *
	 * @param os: the output stream
	 * @param value: the {{type_name}} value to stream
	 * @returns std::ostream
	 */
	std::ostream& operator<<(std::ostream& os, const {{type_name}}& value)
	{
        // ADL is a tricky little bugger...
        using {{ns_package}}::utils::operator<<;

		os << *value << '\n';
		return os;
	}

    /**
	 * @brief equality operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator==(const {{type_name}}& lhs, const {{type_name}}& rhs)
	{
	    {%- if type_info is alias.is_float %}
	    return utils::EssentiallyEqual(lhs.getValue(), rhs.getValue());
	    {%- else %}
		return *lhs == *rhs;
	    {%- endif %}
	}

    /**
	 * @brief non-equality operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator!=(const {{type_name}}& lhs, const {{type_name}}& rhs)
	{
		return !(lhs == rhs);
	}

    /**
	 * @brief less than operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator<(const {{type_name}}& lhs, const {{type_name}}& rhs)
	{
		return *lhs < *rhs;
	}

    /**
	 * @brief less than or equal operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator<=(const {{type_name}}& lhs, const {{type_name}}& rhs)
	{
		return *lhs <= *rhs;
	}

    /**
	 * @brief greater than operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator>(const {{type_name}}& lhs, const {{type_name}}& rhs)
	{
		return *lhs > *rhs;
	}

    /**
	 * @brief greater than or equal operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator>=(const {{type_name}}& lhs, const {{type_name}}& rhs)
	{
		return *lhs >= *rhs;
	}
} // namespace {{ns_tpl}}