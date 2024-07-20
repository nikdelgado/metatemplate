#pragma once

#include "Converter.h"
{%- for header in type_info|variant.includes %}
#include {{header}}
{%- endfor %}

#include "{{path_package}}/{{type_name}}.pb.h"

{%- if type_name is member.has_include_override %}
{%- for file in type_name | member.include_override %}
#include "{{ file }}"
{%- endfor %}
{%- endif %}

using {{type_name}} = std::variant<{%- for choice in type_info|variant.choices %}{%- if loop.last %}{{ns_package}}::{{choice.raw_type}}{%- else %}{{ns_package}}::{{choice.raw_type}}, {%- endif %}{%- endfor %}>;

namespace {{ns_tpl}}
{
    class Convert{{type_name}}
    {
    public:
        static bool to_protobuf({{ns_package}}::{{type_name}}& dest, const {{ns_package}}::{{type_name}}& src);
        static bool from_protobuf({{ns_package}}::{{type_name}}& dest, const {{ns_package}}::{{type_name}}& src);
    };

    template<>
    class Converter<{{ns_package}}::{{type_name}}, {{ns_package}}::{{type_name}}>
    {
    public:
        using type = Convert{{type_name}};
        using protobuf_ns = {{ns_package}}::{{type_name}};
        using cpp_ns = {{ns_package}}::{{type_name}};
    };
} // namespace {{ns_tpl}}