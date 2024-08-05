#include "{{type_name}}.h"

#include "{{path_package}}/utils/PopulateMutex.h"

{%- if type_info.attrs|select('member.is_native')|list %}
#include "{{path_package}}/utils/Native.h"
{%- endif %}

{%- if type_info.attrs|select('member.is_list')|list %}
#include "{{path_package}}/utils/Vector.h"
{%- endif %}

// include dependencies
{%- set included_files = [] %}
{%- for attr in type_info.attrs|select('member.is_custom')|list %}
{%- set include_path = attr|util.include(path_package) %}

{%- if include_path not in included_files %}
{%- set _ = included_files.append(include_path) %}
{%- endif %}

{%- endfor %}

{%- for ex in type_info.extensions %}
{%- set include_path = "\"" + (ex|ext.type) + ".h\"" %}

{%- if include_path not in included_files %}
{%- set _ = included_files.append(include_path) %}
{%- endif %}

{%- endfor %}

{%- for include in (
    type_info.attrs|map('cpp.includes', false)|sum(start=[])
    + type_info.attrs|map('h.includes')|sum(start=[])
)|sort|unique %}
{%- set include_path = include|incl.quote_fix(path_package) %}

{%- if include_path not in included_files %}
{%- set _ = included_files.append(include_path) %}
{%- endif %}

{%- endfor %}

{%- for include_path in included_files|sort|unique %}
#include {{ include_path }}
{%- endfor %}

namespace {{ns_tpl}}
{
    {%- if type_info.attrs or type_info.extensions %}
    bool Convert{{type_name}}::from_protobuf({{ns_package}}::{{type_name}}& dest, const {{ns_package}}::{{type_name}}& src)
    {
        bool success = true;

        const auto lock = std::scoped_lock{utils::populateMutex};

        {%- for attr in type_info.attrs %}
        {%- set protobuf_getter = attr.name.lower().replace("_", "") %}
        {%- set protobuf_setter = "set_" + attr.name.lower().replace("_", "") %}
    
        {%- set cpp_getter = "get" + attr.name | member.set_first_uppercase %}
        {%- set cpp_setter = "set" + attr.name | member.set_first_uppercase %}
        
        {
            {%- if attr is member.is_native and attr is not member.is_list %}
            dest.{{cpp_setter}}(src.{{protobuf_getter}}());

            {%- elif attr is member.is_list and attr is member.is_primitive_list %}
            {%- if "{http://www.w3.org/2001/XMLSchema}double" in attr.types[0].qname %}
            std::vector<double> temp_vector;

            const auto& {{protobuf_getter}} = src.{{protobuf_getter}}();
            std::copy({{protobuf_getter}}.begin(), {{protobuf_getter}}.end(), std::back_inserter(temp_vector));

            {%- else %}
            std::vector<{{ns_package}}::{{attr.types[0].qname}}> temp_vector;

            for (int i = 0; i < src.{{protobuf_getter}}().size(); ++i) {
                temp_vector.push_back(src.{{protobuf_getter}}(i));
            }

            {%- endif %}
            using ConversionType = Converter<
                std::decay_t<decltype(dest.{{cpp_getter}}())>,
                std::decay_t<decltype(temp_vector)>
            >::type;

            auto temp_dest = dest.{{cpp_getter}}();

            success &= ConversionType::from_protobuf(temp_dest, temp_vector);

            dest.{{cpp_setter}}(temp_dest);

            {%- else %}
            using ConversionType = Converter<
                std::decay_t<decltype(dest.{{cpp_getter}}())>, 
                std::decay_t<decltype(src.{{protobuf_getter}}())>
            >::type;
            
            auto temp_dest = dest.{{cpp_getter}}();
            auto temp_src = src.{{protobuf_getter}}();

            success &= ConversionType::from_protobuf(temp_dest, temp_src);

            dest.{{cpp_setter}}(temp_dest);
            {%- endif %}
        }
        {%- endfor %}

        return success;
    }
    {%- else %}
    bool Convert{{type_name}}::from_protobuf({{ns_package}}::{{type_name}}& /*dest*/, const {{ns_package}}::{{type_name}}& /*src*/)
    {
        return true;
    }
    {%- endif %}

    {%- if type_info.attrs or type_info.extensions %}
    bool Convert{{type_name}}::to_protobuf({{ns_package}}::{{type_name}}& dest, const {{ns_package}}::{{type_name}}& src)
    {
        bool success = true;

        const auto lock = std::scoped_lock{utils::populateMutex};

        {%- for attr in type_info.attrs %}
        {%- set protobuf_getter = attr.name.lower().replace("_", "") %}
        {%- set protobuf_setter = "set_" + attr.name.lower().replace("_", "") %}
        {%- set protobuf_mutable = "mutable_" + attr.name.lower().replace("_", "") %}
        {%- set protobuf_adder = "add_" + attr.name.lower().replace("_", "") %}
        {%- set cpp_getter = "get" + attr.name | member.set_first_uppercase %}
        {%- set cpp_setter = "set" + attr.name | member.set_first_uppercase %}
        {

            {%- if attr is member.is_native and attr is not member.is_list %}
            dest.{{protobuf_setter}}(src.{{cpp_getter}}());

            {%- elif attr is member.is_simple and attr is not member.is_list %}
            dest.{{protobuf_setter}}(src.{{cpp_getter}}.getValue());

            {%- elif attr is member.is_list and attr is member.is_primitive_list %}
            {%- if "{http://www.w3.org/2001/XMLSchema}double" in attr.types[0].qname %}
            std::vector<double> temp_vector;
            {%- else %}
            std::vector<{{path_package}}::{{attr.types[0].qname}}> temp_vector;
            {%- endif %}

            using ConversionType = Converter<
                std::decay_t<decltype(src.{{cpp_getter}}())>,
                std::decay_t<decltype(temp_vector)>
            >::type;

            auto temp_src = src.{{cpp_getter}}();

            success &= ConversionType::to_protobuf(temp_vector, temp_src);

            std::for_each(temp_vector.begin(), temp_vector.end(), [&dest] (const auto& element){
                dest.add_{{attr.name.lower().replace("_", "")}}(element);
            });
            {%- else %}
            using ConversionType = Converter<
                std::decay_t<decltype(src.{{cpp_getter}}())>,
                std::decay_t<decltype(dest.{{protobuf_getter}}())> 
            >::type;
            {%- if attr is member.is_enum %}
            auto temp_dest = dest.{{protobuf_getter}}();
            auto temp_src = src.{{cpp_getter}}();

            success &= ConversionType::to_protobuf(temp_dest, temp_src);
            
            dest.{{protobuf_setter}}(temp_dest);
            {%- else %}
            auto temp_dest = dest.{{protobuf_mutable}}();
            const auto &temp_src = src.{{cpp_getter}}();

            success &= ConversionType::to_protobuf(*temp_dest, temp_src);
            {%- endif %}
            {%- endif %}
        }
        {%- endfor %}

        return success;
    }
    {%- else %}
    bool Convert{{type_name}}::to_protobuf({{ns_package}}::{{type_name}}& /*dest*/, const {{ns_package}}::{{type_name}}& /*src*/)
    {
        return true;
    }
    {%- endif %}
} // namespace {{ns_tpl}}