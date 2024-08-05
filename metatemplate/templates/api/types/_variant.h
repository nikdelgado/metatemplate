#pragma once

#include <variant>
#include <iostream>

{%- for header in type_info|variant.includes %}
#include {{header|incl.quote_fix(path_package)}}
{%- endfor %}

namespace {{ns_package}}::byte_stream
{
    class OByteStream;
    class IByteStream;
}

{%- set variant_types = type_info|variant.choices|map(attribute='type')|sort|unique|list -%}

namespace {{ns_tpl}}
{
    /**
     * {{type_info.help|clean_docstring}}
     */
    class {{type_name}}
    {
    public:
		using ChoiceTypes = std::variant<{{variant_types|join(", ")}}>;

	    /**
         * @brief choice for {{field_name}}
         */
        enum class Choice
        {
            {%- for choice in type_info|variant.choices %}
            {{choice.name}}{{"," if not loop.last}}
            {%- endfor %}
        };

        /**
         * @brief Default Constructor
         */
        {{type_name}}() = default;

        {#- If there are no duplicate variant types, support per variant ctor, otherwise, take std::variant + enum... -#}
        {%- if variant_types|length == type_info|variant.choices|length -%}
        {%- for choice in type_info|variant.choices %}
        /**
         * @brief {{choice_name}} Constructor
         */
        {{type_name}}({{choice.type}} value);
        {%- endfor %}
        {%- else %}
        /**
         * @brief Typed Constructor
         */
        {{type_name}}(ChoiceTypes value, Choice choice);
        {%- endif %}

        /**
         * @brief The held choice
         *
         * @return Choice
         */
        [[nodiscard]] Choice heldChoice() const noexcept;

        /**
         * @brief default activate the choice
         * 
         * @note can throw if the default for the type throws in it's default ctor
         *
         * @param choice
         */
        void defaultActivateChoice(Choice choice);

		/**
         * @brief The held choice value
         *
         * @return ChoiceTypes
         */
        [[nodiscard]] const ChoiceTypes& heldValue() const noexcept;

        {%- for choice in type_info|variant.choices %}

        /**
         * @brief tests if the {{choice.name}} is held
         *
         * @return bool true if the {{choice.name}} is the held choice, else false
         */
        bool holds{{choice.name}}() const noexcept;

        /**
         * @brief setter for the {{choice.name}} choice
         *
         * @param value: the {{choice.name}} value
         */
        void set{{choice.name}}({{choice.type}} value);

        /**
         * @brief getter for the {{choice.name}} choice
         *
         * @return {{choice.type}} const reference
         */
        [[nodiscard]] const {{choice.type}}& get{{choice.name}}() const;

        /**
         * @brief getter for the {{choice.name}} choice
         *
         * @return {{choice.type}} reference
         */
        [[nodiscard]] {{choice.type}}& get{{choice.name}}();
		{%- endfor %}

		/**
         * @brief Writes the variant to a byte stream
         *
         * @param bs The bytestream.
         */
        void toByteStream(byte_stream::OByteStream& bs) const;

        /**
         * @brief Reads the variant from a byte stream
         *
         * @param bs The bytestream.
         */
        void fromByteStream(byte_stream::IByteStream& bs);

    private:
        Choice choice_{ Choice::{{type_info|variant.default|attr('name')}} };
        ChoiceTypes value_ = {{type_info|variant.default|attr('type')}}{};
        {%- for choice in type_info|variant.choices %}

        /**
         * @brief getter for the {{choice.name}} choice from a bytestream
         * @param bs: the bytestream.
         * @return {{choice.type}} instance
         */
        [[nodiscard]] {{choice.type}} get{{choice.name}}FromByteStream(byte_stream::IByteStream& bs);
		{%- endfor %}
	};

	/**
	 * @brief equality operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator==(const {{type_name}}& lhs, const {{type_name}}& rhs);
    
	/**
	 * @brief not equals operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the r=lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator!=(const {{type_name}}& lhs, const {{type_name}}& rhs);

    /**
	 * @brief Output stream operator for the {{type_name}}
	 *
	 * @param os: the output stream
	 * @param value: the {{type_name}}
	 * @returns std::ostream
	 */
    std::ostream& operator<<(std::ostream& os, const {{type_name}}& value);
} // namespace {{ns_tpl}}