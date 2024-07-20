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


namespace cpp_ns::types {
    {%- for class_name in classes %}
    {%- if class_name in complex_types %}
    class {{ class_name }};
    {%- endif %}
    {%- endfor %}
} // namespace cpp_ns



namespace {{ns_package}}
{
    template<protobuf_ns::TypeMapEnum T>
    class RecieveMap;

{%- for type_name, _type in class_map|dictsort %}
    template<>
    class RecieveMap<::TypeMapEnum::TypeMapEnum_{{type_name}}>
    {
    public:
        using CppType = cpp_ns::types::{{type_name}};
        using ProtoType = protobuf_ns::types::{{type_name}};
    };
{% endfor %}

    template<class T>
    class SendMap;

{%- for type_name, _type in class_map|dictsort %}

    template<>
    class SendMap<cpp_ns::types::{{type_name}}>
    {
    public:
        const ::types::TypeMapEnum enumType = ::types::TypeMapEnum_{{type_name}};
    };
{% endfor %}
} // namespace {{ns_package}}