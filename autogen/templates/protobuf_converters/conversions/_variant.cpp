#include <variant>
#include "{{type_name}}.h"

{%- if type_name is member.has_include_override %}
{%- for file in type_name | member.include_override %}
#include "{{ file }}"
{%- endfor %}
{%- endif %}

#include "{{path_package}}/utils/PopulateMutex.h"

{%- if type_info|variant.choices|map(attribute='as_attr')|select('member.is_list')|list %}
#include "{{path_package}}/utils/Vector.h"
{%- endif %}

{%- for attr in type_info|variant.choices|map(attribute='as_attr')|select('member.is_custom')|list %}
#include {{attr|util.include(path_package)}}
{%- endfor %}

{%- for header in type_info|variant.includes %}
#include {{header}}
{%- endfor %}

{%- for header in type_info|variant.cpp_includes %}
#include {{header}}
{%- endfor %}

namespace {{ns_tpl}}
{
    bool Convert{{type_name}}::from_protobuf({{path_package}}::{{type_name}}& dest, const {{path_package}}::{{type_name}}& src)
    {
        bool success = true;
        const auto lock = std::scoped_lock{utils::populateMutex};
        switch (src.choice())
        {
            {%- for choice in type_info|variant.choices %}
            case {{path_package}}::{{type_name}}ChoiceType::{{type_name}}Choice_{{choice.name}}: 
            {
                {%-if choice.as_attr is member.is_native and choice.as_attr is not member.is_list %}
                dest.set{{choice.name}}(src.{{choice.name}}());
				break;
                {%- else %}
                auto temp_{{choice.raw_type}} = {{path_package}}::{{choice.raw_type}}();
				using ConversionType =  Converter<
                    std::remove_reference_t<decltype(temp_{{choice.raw_type}})>,
                    std::remove_cv_t<std::remove_reference_t<decltype(src.{{choice.name.lower()}}())>>
                >::type;
                
                success &= ConversionType::from_protobuf(temp_{{choice.raw_type}}, src.{{choice.name.lower()}}());
                dest = temp_{{choice.raw_type}};
                break;
				{%- endif %}
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
   
        {%- for choice in type_info|variant.choices %}
        if (std::holds_alternative<{{path_package}}::{{choice.raw_type}}>(src))
        {
            dest.set_choice({{path_package}}::{{type_name}}ChoiceType::{{type_name}}Choice_{{choice.name}});
            {%- if choice.as_attr is member.is_native and choice.as_attr is not member.is_list %}
            dest.{{choice.name}}(src);
            break;
            {%- else %}
            auto temp_src = std::get<{{path_package}}::{{choice.raw_type}}>(src);
            using ConversionType = Converter<
                std::remove_reference_t<decltype(temp_src)>,
                std::remove_cv_t<std::remove_reference_t<decltype(dest.{{choice.raw_type.lower()}}())>>
            >::type;
            
            auto temp_dest = dest.mutable_{{choice.raw_type.lower()}}();
            return ConversionType::to_protobuf(*temp_dest, temp_src);
            {%- endif %}
        }
        {%- endfor %}        
		return true;
	}
} // namespace {{ns_tpl}}