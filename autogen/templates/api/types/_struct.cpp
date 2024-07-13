#include <cstddef>
#include <cstdint>
#include <iostream>

{%- if type_info.attrs|select("member.is_floating_point")|list|length %}
#include {{"utils/EssentiallyEqual.h" | util_ns.incl}}
{%- endif %}
#include {{"utils/Stream.h" | util_ns.incl}}
#include {{"byte_stream/ByteStream.h" | util_ns.incl}}

{%- set req_attrs = type_info|class.req_attrs %}
{%- set opt_attrs = type_info|class.opt_attrs %}
{%- set ordered_attrs = req_attrs + opt_attrs %}

#include "{{type_name}}_cpp.h"

{%- set imp_class_name = type_info|class.imp_class_name %}
{%- set imp_name = type_info|class.imp_name %}
{%- set has_pimpl = ordered_attrs|length %}

namespace {{ ns_tpl }}
{

    {%- if has_pimpl %}
    class {{ type_name }}::{{ imp_class_name }}
    {
    public:
        {{imp_class_name}}() = default;
        
        {%- if req_attrs %}
        {%- if req_attrs|length == 1 -%}{{'explicit '}}{%- endif -%}
        {{imp_class_name}}({{req_attrs|map('pimpl.ctor_arg')|join(", ")}})
        : 
        {%- for attr in req_attrs %}
        {{attr|member.var_name}}({{attr|member.val_name|member.move_wrap(attr)}}){%- if not loop.last -%},{%- endif -%}
        {%- endfor %}
        {}
        {%- endif %}

        {%- if opt_attrs %}
        {%- if ordered_attrs|length == 1 -%}{{'explicit '}}{%- endif -%}
        {{imp_class_name}}({{ordered_attrs|map('pimpl.ctor_arg')|join(", ")}})
        : 
        {%- for attr in ordered_attrs %}
        {{attr|member.var_name}}({{attr|member.val_name|member.move_wrap(attr)}}){%- if not loop.last -%},{%- endif -%}
        {%- endfor %}
        {}
        {%- endif %}

        {%- for attr in ordered_attrs %}
        /**
         * {{attr.help}}
         */
        {{attr|member.type_name}} {{attr|member.var_name}}{};
        {%- endfor %}

    };
    {%- endif %}

    {#- Default ctor -#}
    {{type_name}}::{{type_name}}()
    {%- if has_pimpl %}
    : {{imp_name}}(std::make_unique<{{imp_class_name}}>())
    {}
    {%- else -%}
    = default;
    {% endif %}
    
    {#- Copy ctor -#}
    {{type_name}}::{{type_name}}(const {{type_name}}& other)
    {#- clang-tidy wants trivial polymorphic copies as = default -#}
    {%- if not has_pimpl -%}
    = default;
    {%- else -%}
    :
    {%- for ex in type_info.extensions %}
    {{ex|ext.type}}(dynamic_cast<const {{ex|ext.type}}&>(other)){%- if not loop.last -%},{%- endif -%}
    {%- endfor %}
    {%- if has_pimpl %}
    {%-   if type_info.extensions|length -%},{%- endif -%}
    {{imp_name}}{std::make_unique<{{ imp_class_name }}>()}
    {%- endif %}
    {
        {%- for attr in type_info.attrs %}
        {{imp_name}}->{{attr|member.var_name}} = other.{{imp_name}}->{{attr|member.var_name}};
        {%- endfor %}
    }
    {% endif %}
    
    {#- Move ctor, always default, unique_ptr knows how to move #}
    {{type_name}}::{{type_name}}({{type_name}}&& other) noexcept = default;

    {%- if req_attrs + type_info.extensions %}
    {{type_name}}::{{type_name}}({{type_info|class.req_ctor_args}})
    : {{type_info|class.req_ctor_init}}
    {}
    {%- endif %}

    
    {%- if ordered_attrs|length > req_attrs|length %}
    {{type_name}}::{{type_name}}({{type_info|class.all_ctor_args_cpp}})
    : {{type_info|class.all_ctor_init}}
    {}
    {%- endif %}
    
    {{type_name}}::~{{type_name}}() = default;


    {%- for attr in ordered_attrs %}

    {%- if attr is member.is_optional_type %}

    {{ type_name }}& {{ type_name}}::{{ attr|member.clearer}}()
    {
        {{ imp_name }}->{{ attr|member.var_name }} = std::nullopt;
        return *this;
    }

    
    {{ type_name }}& {{ type_name}}::{{ attr|member.setter}}Opt({{ attr|member.type_name}} {{ attr|member.val_name}})
    {
        {{ imp_name }}->{{ attr|member.var_name }} = {{ attr|member.val_name|member.move_wrap(attr)}};
        return *this;
    }

    // Extra setter for non-optional type
    {{ type_name }}& {{ type_name}}::{{ attr|member.setter}}({{ attr|member.no_opt_type_name}} {{ attr|member.val_name}})
    {
        {{ imp_name }}->{{ attr|member.var_name }} = {{ attr|member.val_name|member.move_wrap(attr)}};
        return *this;
    }
    {%- else %}

    {{ type_name }}& {{ type_name}}::{{ attr|member.setter}}({{ attr|member.type_name}} {{ attr|member.val_name}})
    {
        {{ imp_name }}->{{ attr|member.var_name }} = {{ attr|member.val_name|member.move_wrap(attr)}};
        return *this;
    }
    
    {%- endif %}


    {{ attr|member.cref_type_name}} {{ type_name}}::{{ attr|member.getter}}() const
    {
        return {{ imp_name }}->{{ attr|member.var_name }};
    }
    

    {%- if not attr.native_types %}
    {{ attr|member.ref_type_name}} {{ type_name}}::{{ attr|member.getter}}()
    {
        return {{ imp_name }}->{{ attr|member.var_name }};
    }


    {%- endif %}
    {%- endfor %}

    {%- for ex_name, attrs in type_info|class.inherited_attrs|dictsort -%}
    {%-   for attr in attrs -%}
    {%-     if attr is member.is_optional_type %}
    {# Optional types have a opt and non-opt setter #}
    

    {{ type_name }}& {{ type_name}}::{{ attr|member.clearer}}()
    {
        this->{{ex_name}}::{{ attr|member.clearer}}();
        return *this;
    }

    {{ type_name }}& {{ type_name}}::{{ attr|member.setter}}Opt({{ attr|member.type_name}} {{ attr|member.val_name}})
    {
        this->{{ex_name}}::{{ attr|member.setter}}Opt({{ attr|member.val_name}});
        return *this;
    }

    // Extra setter for non-optional type
    {{ type_name }}& {{ type_name}}::{{ attr|member.setter}}({{ attr|member.no_opt_type_name}} {{ attr|member.val_name}})
    {
        this->{{ex_name}}::{{ attr|member.setter}}({{ attr|member.val_name}});
        return *this;
    }

    {%-     else %}
    {# Non-optional types, just one setter #}

    {{ type_name }}& {{ type_name}}::{{ attr|member.setter}}({{ attr|member.type_name}} {{ attr|member.val_name}})
    {
        this->{{ex_name}}::{{ attr|member.setter}}({{ attr|member.val_name}});
        return *this;
    }
    {%-     endif %}
    {%-   endfor %}
    {%- endfor %}


    void {{ type_name }}::toByteStream(byte_stream::OByteStream& bs) const
    {
        bs << ID();
        
        {%- for ex in type_info.extensions -%}
        {
            {{ex|ext.type}}::toByteStream(bs);
        }
        {%- endfor -%}

        {%- for attr in ordered_attrs %}
        {%- if loop.first %}
        bs
        {%- endif %}
            << {{ imp_name }}->{{ attr|member.var_name }}{%- if loop.last -%};{%- endif -%}
        {%-endfor %}
    }

    void {{ type_name }}::fromByteStream(byte_stream::IByteStream& bs)
    {
        std::remove_const_t<decltype(ID())> id{};
        bs >> id;
        if (id != ID())
        {
            throw std::runtime_error("ID:" + std::to_string(id)
                + " of the bytestream does not match the class ID: "
                + std::to_string(ID())
                + " for message {{type_name}}");
        }

        {%- for ex in type_info.extensions -%}
        {
            {{ex|ext.type}}::fromByteStream(bs);
        }
        {%- endfor -%}

        {%- for attr in ordered_attrs %}
        {%- if loop.first %}
        bs
        {%- endif %}
            >> {{ imp_name }}->{{ attr|member.var_name }}{%- if loop.last -%};{%- endif -%}
        {%-endfor %}
    }

    
    std::vector<std::byte> {{ type_name }}::serialize() const
    {
        byte_stream::OByteStream bs;
        bs << *this;
        return bs.buffer();
    }

    {{type_name}} {{type_name}}::deserialize(const void* bufferPtr, std::size_t bufferSize)
    {
        byte_stream::IByteStream bs(static_cast<const std::byte*>(bufferPtr), bufferSize);
        auto obj = {{type_name}} {};
        bs >> obj;
        return obj;
    }

    {{type_name}} {{ type_name }}::deserialize(const std::vector<std::byte>& bytes)
    {
        return deserialize(bytes.data(), bytes.size());
    }

    {{type_name}}& {{type_name}}::operator=(const {{type_name}}& other) noexcept
    {#- clang-tidy wants trivial polymorphic copy assign as = default -#}
    {%- if not has_pimpl -%}
    = default;
    {%- else %}
    {
        // private implementation hides all internals from
        // inherited classes, each parent needs the operator executed
        {%- for ex in type_info.extensions %}
        {{ex|ext.type}}::operator=(other);
        {%- endfor %}
        
        {%- for attr in type_info.attrs %}
        {{ imp_name }}->{{ attr|member.var_name }} = other.{{ imp_name }}->{{ attr|member.var_name }};
        {%- endfor %}

        return *this;
    }
    {% endif %}

    {{type_name}}& {{type_name}}::operator=({{type_name}}&& other) noexcept
    {#- clang-tidy wants trivial polymorphic move assign as = default -#}
    {%- if not has_pimpl -%}
    = default;
    {%- else %}
    {
        // private implementation hides all internals from
        // inherited classes, each parent needs the operator executed
        {%- for ex in type_info.extensions %}
        {{ex|ext.type}}::operator=(other);
        {%- endfor %}

        {{imp_name}}.swap(other.{{imp_name}});

        return *this;
    }
    {%- endif %}

	bool operator==(const {{type_name}}& lhs, const {{type_name}}& rhs)
	{
        return
        {%- for check in type_info|class.equality_checks('lhs', 'rhs') %}
            {{check}}
        {{" &&" if not loop.last else ";"}}
        {%- endfor %}
	}
    
    
	bool operator!=(const {{type_name}}& lhs, const {{type_name}}& rhs)
	{
        return !(lhs == rhs);
    }

    {%- if type_info is class.extends_abstract %}
    bool {{type_name}}::equals(const {{type_info|class.abstract_base|attr('name')}}& other) const
    {
        // just checks local attrs for polymorphic checking from parent base class
        // parent already includes everything else, including the id == id at the base, before any subclass dynamic cassts.
        if (!{{type_info|class.abstract_parent|attr('name')}}::equals(other)) return false;
        const {{type_name}}& rhs = dynamic_cast<const {{type_name}}&>(other);
        return {{ "true;" if not type_info.attrs }}
	    {%-   for attr in type_info.attrs -%}
        {%-     set field_access = attr|member.getter -%}
	    {%-     if attr is member.is_floating_point %}
	    utils::EssentiallyEqual({{field_access}}(), rhs.{{field_access}}())
	    {%-     else %}
	    {{field_access}}() == rhs.{{field_access}}()
	    {%-     endif %}
        {{" &&" if not loop.last else ";"}}
        {%-   endfor %}
    }
    {%- endif %}

    {% set type_instance = type_name|names.val_name -%}
    /**
	 * @brief Output stream operator for the {{type_name}}
	 *
	 * @param os: the output stream
	 * @param {{type_instance}}: the {{type_name}} instance
	 * @returns std::ostream
	 */
    std::ostream& operator<<(std::ostream& os, const {{type_name}}& {{type_instance}})
    {
        using {{ns_package}}::utils::operator<<;

        os << "{{type_name}}.begin:\n";
        {%- for ex in type_info.extensions -%}
        os << "\t{{type_name}}.extension[{{loop.index}}].begin:\n";
        os << dynamic_cast<const {{ex|ext.type}}&>({{type_instance}}) << "\n";  
        os << "\t{{type_name}}.extension[{{loop.index}}].end\n";
        {%- endfor %}
        {%- for attr in type_info.attrs %}
        os << "\t{{attr.name}}: " << {{type_instance}}.{{attr|member.getter}}() << "\n";          
        {%- endfor %}
        os << "{{type_name}}.end:\n";
        return os;
    }

} // namespace {{ ns_tpl }}
