#pragma once

#include "Converter.h"
#include "{{path_package}}/{{type_name}}.pb.h"

{%- if type_name is member.has_include_override %}
{%- for file in type_name | member.include_override %}
#include "{{ file }}"
{%- endfor %}
{%- endif %}

namespace {{ns_tpl}}
{
    class Convert{{type_name}}
    {
    public:
        static bool to_protobuf({{path_package}}::{{type_name}}& dest, const {{path_package}}::{{type_name}}& src);
        static bool from_protobuf({{path_package}}::{{type_name}}& dest, const {{path_package}}::{{type_name}}& src);
    };

    template<>
    class Converter<{{path_package}}::{{type_name}}, {{path_package}}::{{type_name}}>
    {
    public:
        using type = Convert{{type_name}};
        using protobuf_ns = {{path_package}}::{{type_name}};
        using cpp_ns = {{path_package}}::{{type_name}};
    };
} // namespace {{ns_tpl}}