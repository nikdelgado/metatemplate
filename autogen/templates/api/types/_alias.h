#pragma once

#include <iostream>
{%- if type_info is alias.is_string %}
#include <string_view>
{%- endif %}

{%- for include in type_info|alias.includes %}
#include {{include|incl.quote_fix(path_package)}}
{%- endfor %}

namespace {{ns_package}}::byte_stream {
    class OByteStream;
    class IByteStream;
}

namespace {{ns_tpl}}
{

    /**
     * {{type_info.help|clean_docstring}}
     */
    class {{type_name}}
    {
    public:
	
        using alias_type = {{type_info|alias.primitive}};
		using ref_type = alias_type&;
		using const_ref_type = const alias_type&;

		
		{%- set const_type = "const" if type_info is alias.is_string else "constexpr" %}

        {%- set field_name = "min_inclusive" %}
        {%- if type_info is alias.has_restriction(field_name) %}
        static inline const alias_type {{field_name.upper()}} { {{type_info|alias.restriction(field_name)}} };
		{%- endif %}
        {%- set field_name = "min_exclusive" %}
        {%- if type_info is alias.has_restriction(field_name) %}
        static inline const alias_type {{field_name.upper()}} { {{type_info|alias.restriction(field_name)}} };
		{%- endif %}
        {%- set field_name = "max_inclusive" %}
        {%- if type_info is alias.has_restriction(field_name) %}
        static inline const alias_type {{field_name.upper()}} { {{type_info|alias.restriction(field_name)}} };
		{%- endif %}
        {%- set field_name = "max_exclusive" %}
        {%- if type_info is alias.has_restriction(field_name) %}
        static inline const alias_type {{field_name.upper()}} { {{type_info|alias.restriction(field_name)}} };
		{%- endif %}
        {%- set field_name = "length" %}
        {%- if type_info is alias.has_restriction(field_name) %}
        static constexpr std::size_t {{field_name.upper()}} { {{type_info|alias.restriction(field_name)}} };
		{%- endif %}
        {%- set field_name = "min_length" %}
        {%- if type_info is alias.has_restriction(field_name) %}
        static constexpr std::size_t {{field_name.upper()}} { {{type_info|alias.restriction(field_name)}} };
		{%- endif %}
        {%- set field_name = "max_length" %}
        {%- if type_info is alias.has_restriction(field_name) %}
        static constexpr std::size_t {{field_name.upper()}} { {{type_info|alias.restriction(field_name)}} };
		{%- endif %}
        
        {%- if type_info is alias.is_string -%}
        {%-   set field_name = "pattern" %}
        {%-   if type_info is alias.has_restriction(field_name) %}
        static inline const alias_type {{field_name.upper()}} { {{type_info|alias.restriction(field_name)|tojson}} };
		{%-   endif %}
		{%- endif %}
        
        /**
         * @brief Default Constructor
         */
        {%- if type_info is alias.has_default %}
		{{type_name}}() : value_{ {{type_info|alias.default}} }
		{
		}
		{%- else %}
		{{type_name}}() = default;
		{%- endif %}

		/**
		 * @brief Copy Constructor
         * @param other
		 */
		{{type_name}}(const {{type_name}}& other) noexcept = default;

		/**
		 * @brief Move Constructor
         * @param other
		 */
		{{type_name}}({{type_name}}&& other) noexcept = default;

		/**
		 * @brief Value checking Constructor
         * @param value
		 */
		{{type_name}}(const_ref_type value)
		: value_(checkValue(value))
		{
			setValue(value);
		}

        /**
		 * @brief dereference operator to get the underlying value
		 *
		 * @return value
		 */
        const_ref_type operator*() const noexcept;

        /**
		 * @brief underlying value
		 *
		 * @return value
		 */
        const_ref_type getValue() const noexcept;

		/**
		 * @brief value setter
		 *
		 * @param val
		 */
		void setValue(const_ref_type val);

	    /*
	     * @brief Conversion operator to convert the class to its underlying type
	     */
	     operator alias_type() const;

		/*
         * @brief Writes the alias to a byte stream
         *
         * @param bs The bytestream.
         */
        void toByteStream(byte_stream::OByteStream& bs) const;

        /*
         * @brief Reads the alias from a byte stream
         *
         * @param bs The bytestream.
         */
        void fromByteStream(byte_stream::IByteStream& bs);

        /**
         * @brief Copy assignment operator
         *
         * @return this
         */
        {{type_name}}& operator=(const {{type_name}}& bytes) noexcept = default;

        /**
         * @brief Move assignment operator
         *
         * @return this
         */
        {{type_name}}& operator=({{type_name}}&& other) noexcept = default;

    private:
        /*
         * @brief Constraints check with constexpr support
         *
         * @param val Value to check bounds
		 * @return val if no assertion fails
		 * @throws std::invalid_argument if constraint fails
         */
		static const_ref_type checkValue(const_ref_type val);

		alias_type value_{};
	};

    /**
	 * @brief Output stream operator for {{type_name}}
	 *
	 * @param os: the output stream
	 * @param value: the {{type_name}} value to stream
	 * @returns std::ostream
	 */
	std::ostream& operator<<(std::ostream& os, const {{type_name}}& value);

    /**
	 * @brief equality operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator==(const {{type_name}}& lhs, const {{type_name}}& rhs);

    /**
	 * @brief non-equality operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator!=(const {{type_name}}& lhs, const {{type_name}}& rhs);

    /**
	 * @brief less than operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator<(const {{type_name}}& lhs, const {{type_name}}& rhs);
    /**
	 * @brief less than or equal operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator<=(const {{type_name}}& lhs, const {{type_name}}& rhs);

    /**
	 * @brief greater than operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator>(const {{type_name}}& lhs, const {{type_name}}& rhs);

    /**
	 * @brief greater than or equal operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator>=(const {{type_name}}& lhs, const {{type_name}}& rhs);
} // namespace {{ns_tpl}}