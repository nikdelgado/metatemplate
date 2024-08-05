#include {{"byte_stream/ByteStream.h" | util_ns.incl}}

{%- for subclass in derived %}
#include {{subclass.name | class_name.include}}
{%- endfor %}

#include "{{type_name}}Factory.h"

namespace {{ ns_tpl }} {

std::shared_ptr<{{type_name}}> {{type_name}}AbstractFactoryImpl::from_stream(byte_stream::IByteStream& bs)
{
    std::remove_const_t<decltype({{type_name}}::ID())> id{};
    bs >> id;
    if (id == 0u) return {};

    {%- for subclass in derived %}
    {{ "else " if not loop.first }}if ( id == {{subclass.name}}::ID()) {
        {{subclass.name}} sc{};
        bs >> sc;
        return std::make_shared<{{subclass.name}}>(sc);
    }
    {%- endfor %}
    else {
        throw std::runtime_error("ID: " + std::to_string(id) + " Invalid for {{type_name}}");
    }

}

void {{type_name}}AbstractFactoryImpl::to_stream(std::shared_ptr<{{type_name}}> obj, byte_stream::OByteStream& bs)
{
    // just puts the ID twice to simplify the read above
    const decltype({{type_name}}::ID()) id{obj ? obj->abstractId(): 0u};
    bs << id;
    if (!obj) return;
    {%- for subclass in derived %}
    {{ "else " if not loop.first }}if ( id == {{subclass.name}}::ID()) {
        bs << *std::dynamic_pointer_cast<{{subclass.name}}>(obj);
    }
    {%- endfor %}
    else {
        throw std::runtime_error("ID: " + std::to_string(id) + " Invalid for {{type_name}}");
    }
}

} // namespace {{ ns_tpl }}