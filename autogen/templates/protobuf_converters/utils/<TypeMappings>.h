#pragma once

#include <string_view>
#include "protobuf_ns/types/TypeMapEnum.pb.h"

namespace protobuf_ns::types
{
{%- set complex_types = [] %}
{%- for type_name, _type in class_map|dictsort %}
class {{type_name}};
{%- endfor %}
} // namespace protobuf_ns::types


namespace {{ namespace }} {
    {%- for class_name in classes %}
    {%- if class_name in complex_types %}
    class {{ class_name }};
    {%- endif %}
    {%- endfor %}
} // namespace {{ namespace }}



namespace {{ns_package}}
{
    template<protobuf_ns::TypeMapEnum T>
    class RecieveMap;

{%- for type_name, _type in class_map|dictsort %}
    template<>
    class RecieveMap<::TypeMapEnum::TypeMapEnum_{{type_name}}>
    {
    public:
        using CppType = {{type_name}};
        using ProtoType = ::types::{{type_name}};
    };
{% endfor %}

    template<class T>
    class SendMap;

{%- for type_name, _type in class_map|dictsort %}

    template<>
    class SendMap<{{type_name | member.ns_override}}{{type_name}}>
    {
    public:
        const ::types::TypeMapEnum enumType = ::types::TypeMapEnum_{{type_name}};
    };
{% endfor %}
} // namespace {{ns_package}}