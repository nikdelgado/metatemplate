#pragma once

#include <memory>
#include <vector>

{%- for ex in type_info.extensions %}
#include {{ex | ext.type | class_name.include}}
{%- endfor %}

{%- for include in type_info.attrs|map('h.includes')|sum(start=[])|sort|unique %}
#include {{include|incl.quote_fix(path_package)}}
{%- endfor %}

{%- set req_attrs = type_info|class.req_attrs %}
{%- set opt_attrs = type_info|class.opt_attrs %}
{%- set ordered_attrs = req_attrs + opt_attrs %}

namespace {{ns_package}}::byte_stream
{
    class OByteStream;
    class IByteStream;
} // namespace {{ns_package}}::byte_stream

namespace {{ns_tpl}}
{
    {%- for fwd_decl in type_info.attrs|select('member.can_fwd_decl')|map('member.fwd_decl')|sort|unique %}
    {{ fwd_decl }}
    {%- endfor %}

    /**
	 * {{type_info.help|clean_docstring}}
	 */
    class {{type_name}}{%-if type_info.extensions|length %} :{%- for ex in type_info.extensions %} public {{ex|ext.type}}
    {%-if not loop.last %},{%- endif %}{%- endfor %}{%- endif %}
    {
    public:
        /**
         * @brief Default Constructor
         */
        {{type_name}}();

        /**
         * @brief Copy Constructor
         */
        {{type_name}}(const {{type_name}}& other);

        /**
         * @brief Move Constructor
         */
        {{type_name}}({{type_name}}&& other) noexcept;
        
        /**
         * @brief Copy assignment operator
         *
         * @return this
         */
        {{type_name}}& operator=(const {{type_name}}& bytes) noexcept;

        /**
         * @brief Move assignment operator
         *
         * @return this
         */
        {{type_name}}& operator=({{type_name}}&& other) noexcept;

        /**
         * @brief Destructor
         * 
         * @note must be defined in cpp with private implementation pattern
         */
        virtual ~{{type_name}}();
        

        {%- if req_attrs + type_info.extensions %}
        /**
         * @brief Constructor with all required parameters.
         */
        {{ "explicit " if (req_attrs + type_info.extensions)|length == 1 }}{{type_name}}({{type_info|class.req_ctor_args}});
        {%- endif %}

        
        {%- if ordered_attrs|length > req_attrs|length %}
        /**
         * @brief Constructor with all parameters, optionals have nullopt defaults.
         */
        {{ "explicit " if (ordered_attrs + type_info.extensions)|length == 1 }}{{type_name}}({{type_info|class.all_ctor_args_cpp}});

        {%- endif %}

        /**
         * @brief Returns hard-coded class ID.
         * 
         * @return Hard-coded ID
         */
		static constexpr uint32_t ID()
        {
            return {{type_info|class.id}}u;
        }

        {%- if type_info is class.extends_abstract %}
        /**
         * @brief Returns hard-coded class ID. Used for polymorphic type checking at runtime.
         * 
         * @return Hard-coded ID
         */
        uint32_t abstractId() const override
        {
            return {{type_info|class.id}}u;
        }
        /**
         * @brief polymorphic equality check for abstract classes
         */
        bool equals(const {{type_info|class.abstract_base|attr('name')}}& other) const override;
        {%- elif type_info is class.is_abstract %}
        /**
         * @brief polymorphic equality check for abstract classes
         * 
         * @note this versions shouldn't get called because the abstract classes should not have
         * implemented instances. Not pure virtual due to compiler issues.
         */
        virtual bool equals(const {{type_name}}& other) const
        {
            return this->abstractId() == other.abstractId();
        }
        /**
         * @brief Returns hard-coded class ID. Used for polymorphic type checking at runtime.
         * 
         * @return Hard-coded ID
         */
        virtual uint32_t abstractId() const
        {
            return {{type_info|class.id}}u;
        }
        {%- endif %}

        {%- for attr in type_info.attrs %}

        /**
         * @brief Const getter for {{attr.name}}
         * 
         * {{attr.help|clean_docstring}}
         *
         * @return {{attr.name}}
         */
        [[nodiscard]] {{ attr|member.cref_type_name}} {{ attr|member.getter}}() const;

        {%- if not attr.native_types %}
        /**
         * @brief Reference Getter for {{attr.name}}
         * 
         * {{attr.help|clean_docstring}}
         *
         * @return {{attr.name}}
         */
        [[nodiscard]] {{ attr|member.ref_type_name}} {{ attr|member.getter}}();
        {%- endif %}

        {%- if attr is member.is_optional_type %}
        {# Optional types have a opt and non-opt setter #}
        
        /**
         * @brief Optional setter for {{attr.name}}
         *
         * @param value Sets or clears {{attr.name}}.
         * @return reference to self
         */
        {{ type_name }}& {{ attr|member.setter}}Opt({{ attr|member.type_name}} {{ attr|member.val_name}});

        /**
         * @brief Clear {{attr.name}}
         * 
         * @return reference to self
         */
        {{ type_name }}& {{ attr|member.clearer}}();

        /**
         * @brief Setter for {{attr.name}}
         * 
         * {{attr.help|clean_docstring}}
         *
         * @param value Sets {{attr.name}}.
         * @return reference to self
         */
        {{ type_name }}& {{ attr|member.setter}}({{ attr|member.no_opt_type_name}} {{ attr|member.val_name}});

        {%- else %}
        {# Non-optional types, just one setter #}

        /**
         * @brief Setter for {{attr.name}}
         * 
         * {{attr.help|clean_docstring}}
         *
         * @param value Sets {{attr.name}}.
         * @return reference to self
         */
        {{ type_name }}& {{ attr|member.setter}}({{ attr|member.type_name}} {{ attr|member.val_name}});

        {%- endif %}
		{%- endfor %}

        {%- for ex_name, attrs in type_info|class.inherited_attrs|dictsort -%}
        {%-   for attr in attrs -%}
        {%-     if attr is member.is_optional_type %}
        {# Optional types have a opt and non-opt setter #}
        /**
         * @brief Optional setter for {{attr.name}}
         * 
         * @note shadows {{ex_name}}::{{ attr|member.setter}}Opt to return chainable typing.
         *
         * @param value Sets or clears {{attr.name}}.
         * @return reference to self
         */
        {{ type_name }}& {{ attr|member.setter}}Opt({{ attr|member.type_name}} {{ attr|member.val_name}});

        /**
         * @brief Clear {{attr.name}}
         * 
         * @note shadows {{ex_name}}::{{ attr|member.clearer}}() to return chainable typing.
         * 
         * @return reference to self
         */
        {{ type_name }}& {{ attr|member.clearer}}();

        /**
         * @brief Setter for {{attr.name}}
         * 
         * {{attr.help|clean_docstring}}
         *
         * @note shadows {{ex_name}}::{{ attr|member.setter}} to return chainable typing.
         * 
         * @param value Sets {{attr.name}}.
         * @return reference to self
         */
        {{ type_name }}& {{ attr|member.setter}}({{ attr|member.no_opt_type_name}} {{ attr|member.val_name}});
        {%-     else %}
        {# Non-optional types, just one setter #}

        /**
         * @brief Setter for {{attr.name}}
         * 
         * {{attr.help|clean_docstring}}
         * 
         * @note shadows {{ex_name}}::{{ attr|member.setter}} to return chainable typing.
         *
         * @param value Sets {{attr.name}}.
         * @return reference to self
         */
        {{ type_name }}& {{ attr|member.setter}}({{ attr|member.type_name}} {{ attr|member.val_name}});
        {%-     endif %}
        {%-   endfor %}
        {%- endfor %}

        /**
         * @brief Writes the structure to a byte stream
         *
         * @param bs The bytestream.
         */
        void toByteStream(byte_stream::OByteStream& bs) const;
        /**
         * @brief Reads the structure from a byte stream
         *
         * @param bs The bytestream.
         */
        void fromByteStream(byte_stream::IByteStream& bs);
        /**
         * @brief Serializes the object
         *
         * @return std::vector<std::byte>
         */
        [[nodiscard]] std::vector<std::byte> serialize() const;

        /**
         * @brief Serializes the object
         *
         * @return std::vector<std::byte>
         */
        [[nodiscard]] static {{type_name}} deserialize(const void* bufferPtr, std::size_t bufferSize);

        /**
         * @brief Serializes the object
         *
         * @return std::vector<std::byte>
         */
        [[nodiscard]] static {{type_name}} deserialize(const std::vector<std::byte>& bytes);

    {% if type_info.attrs %}
    private:
        /**
         * @brief Private implementation pattern to reduce exposed
         * scope. See https://en.cppreference.com/w/cpp/language/pimpl
         */
        class {{ type_info|class.imp_class_name }};
        std::unique_ptr<{{ type_info|class.imp_class_name }}> {{ type_info|class.imp_name }};
    {%- endif %}
    };

    /**
	 * @brief equality operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator==(const {{type_name}}& lhs, const {{type_name}}& rhs);
    
    /**
	 * @brief not equals operator for {{type_name}}
	 *
	 * @param rhs: the rhs {{type_name}} value
	 * @param lhs: the lhs {{type_name}} value
	 * @returns bool
	 */
	bool operator!=(const {{type_name}}& lhs, const {{type_name}}& rhs);

    /**
	 * @brief Output stream operator for the {{type_name}}
	 *
	 * @param os: the output stream
	 * @param {{type_instance}}: the {{type_instance}} instance
	 * @returns std::ostream
	 */
    std::ostream& operator<<(std::ostream& os, const {{type_name}}& {{type_name|names.val_name}});

} // namespace {{ns_tpl}}
