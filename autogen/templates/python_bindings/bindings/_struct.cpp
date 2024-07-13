#include <iostream>
#include <sstream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <{{path_api}}/types/{{type_name}}_cpp.h>

{%- set req_attrs = type_info|class.req_attrs %}
{%- set opt_attrs = type_info|class.opt_attrs %}
{%- set ordered_attrs = req_attrs + opt_attrs %}

{%- set inherited_attrs = type_info|class.inherited_attrs|dict.values|sum(start=[]) %}
{%- set all_attrs = ordered_attrs + inherited_attrs %}

namespace py = pybind11;
using namespace {{ns_api}}; // for utils, all prefixed with utils::
using namespace {{ns_api}}::types;

void bind{{type_name}}(py::module_& m)
{
    py::class_<{{type_name}}>(m, "{{type_name}}", py::module_local())
        .def(py::init<>())
        {% if ordered_attrs|length > req_attrs|length -%}
        .def(py::init<{{type_info|class.all_ctor_types}}>())
        {% endif -%}
        {%- if req_attrs + type_info.extensions -%}
        .def(py::init<{{type_info|class.req_ctor_types}}>())
        {% endif -%}
        {% for attr in all_attrs -%}
        .def_property("{{attr|member.val_name}}",
            py::overload_cast<>(&{{type_name}}::{{attr|member.getter}},py::const_),
            &{{type_name}}::{{attr|member.setter}}
        )
        {% endfor -%}
        .def("serialize", &{{type_name}}::serialize)
        .def_static("deserialize", &{{type_name}}::deserialize)
        .def("__eq__", [](const {{type_name}}& lhs, const {{type_name}}& rhs) {return lhs == rhs;})
        .def("__repr__",
            [](const {{type_name}}& a) {
                std::stringstream ss;
                ss << a;
                return ss.str();
            }
        );
}