#include <stdexcept>

#include {{"utils/Stream.h" | util_ns.incl}}
#include {{"byte_stream/ByteStream.h" | util_ns.incl}}

{%- for header in type_info|variant.cpp_includes %}
#include {{header}}
{%- endfor %}

#include "{{type_name}}.h"

{%- set variant_types = type_info|variant.choices|map(attribute='type')|sort|unique|list %}

namespace {{ns_tpl}}
{
    {#- If there are no duplicate variant types, support per variant ctor, otherwise, take std::variant + enum... -#}
    {%- if variant_types|length == type_info|variant.choices|length -%}
    {%- for choice in type_info|variant.choices %}
    {{type_name}}::{{type_name}}({{choice.type}} value)
    {#- Clang-tidy calls setting the default redundant here -#}
    : {%- if choice.name != type_info.default -%}choice_(Choice::{{choice.name}}), {%- endif -%} value_(value)
    {}
    {%- endfor %}
    {%- else %}
    {{type_name}}::{{type_name}}(ChoiceTypes value, Choice choice)
    : choice_(choice), value_({{"value"|member.move_wrap_any(type_info|variant.choices|map(attribute='as_attr'))}})
    {}
    {%- endif %}

    [[nodiscard]] {{type_name}}::Choice {{type_name}}::heldChoice() const noexcept
    {
        return choice_;
    }

    void {{type_name}}::defaultActivateChoice(Choice choice)
    {
        choice_ = choice;
        switch (choice)
        {
            {%- for choice_type, choice_names  in type_info|variant.rolled_types|dictsort %}
            {%- for choice in choice_names %}
            case Choice::{{choice}}:
            {% endfor %}
            {
                value_ = {{choice_type}} {};
                break;
            }
            {%- endfor %}
        }
    }

    [[nodiscard]] const {{type_name}}::ChoiceTypes& {{type_name}}::heldValue() const noexcept
    {
        return value_;
    }

    {%- for choice in type_info|variant.choices %}

    bool {{type_name}}::holds{{choice.name}}() const noexcept
    {
        return choice_ == {{type_name}}::Choice::{{choice.name}};
    }

    void {{type_name}}::set{{choice.name}}({{choice.type}} value)
    {
        choice_ = Choice::{{choice.name}};
        value_ = {{ "value"|member.move_wrap(choice.as_attr) }};
    }

    [[nodiscard]] const {{choice.type}}& {{type_name}}::get{{choice.name}}() const
    {
        if (choice_ != Choice::{{choice.name}})
        {
            throw std::invalid_argument("{{type_name}} does not currently hold a {{choice.name}} choice!");
        }

        return std::get<{{choice.type}}>(value_);
    }

    [[nodiscard]] {{choice.type}}& {{type_name}}::get{{choice.name}}()
    {
        if (choice_ != Choice::{{choice.name}})
        {
            throw std::invalid_argument("{{type_name}} does not currently hold a {{choice.name}} choice!");
        }

        return std::get<{{choice.type}}>(value_);
    }
    {%- endfor %}

    void {{type_name}}::toByteStream(byte_stream::OByteStream& bs) const
    {
        bs << choice_;
        switch(choice_)
        {
        {% for choice in type_info|variant.choices %}
            case Choice::{{choice.name}}:
                bs << get{{choice.name}}();
                break;
        {%- endfor %}
        }
    }

    void {{type_name}}::fromByteStream(byte_stream::IByteStream& bs)
    {
        bs >> choice_;
        switch(choice_)
        {
        {% for choice in type_info|variant.choices %}
            case Choice::{{choice.name}}:
                set{{choice.name}}(get{{choice.name}}FromByteStream(bs));
                break;
        {%- endfor %}
        }
    }

    {%- for choice in type_info|variant.choices %}
    {{choice.type}} {{type_name}}::get{{choice.name}}FromByteStream(byte_stream::IByteStream& bs)
    {
        {{choice.type}} instance{};
        bs >> instance;
        return instance;
    }
    {%- endfor %}

	bool operator==(const {{type_name}}& lhs, const {{type_name}}& rhs)
	{
	    return lhs.heldChoice() == rhs.heldChoice() && lhs.heldValue() == rhs.heldValue();
	}
    
	bool operator!=(const {{type_name}}& lhs, const {{type_name}}& rhs)
	{
        return !(lhs == rhs);
    }

    std::ostream& operator<<(std::ostream& os, const {{type_name}}& value)
    {
        // ADL is a tricky little bugger...
        using {{ns_package}}::utils::operator<<;

		switch(value.heldChoice())
		{
            {%- for choice in type_info|variant.choices %}
            case {{type_name}}::Choice::{{choice.name}}:
            {
				os << "{{choice.name}}: " << value.get{{choice.name}}();
				break;
			}
			{%- endfor %}
            default:
            {
				os << "unknown";
			}
		}

		return os;
    }
} // namespace {{ns_tpl}}