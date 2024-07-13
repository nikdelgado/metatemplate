#include <iostream>
#include <sstream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <{{path_api}}/types/{{type_name}}.h>

namespace py = pybind11;
using namespace {{ns_api}}::types;

{%- set class_name = type_name|names.val_name  %}

void bind{{type_name}}(py::module_& m)
{
    py::class_<{{type_name}}> {{class_name}}(m, "{{type_name}}", py::module_local());
    {{class_name}}
        .def(py::init<>())
        .def_property_readonly("heldChoice", &{{type_name}}::heldChoice)
        .def_property_readonly("heldValue", &{{type_name}}::heldValue)
        .def("defaultActivateChoice", &{{type_name}}::defaultActivateChoice)
        {%- for choice in type_info|variant.choices %}
        .def_property("{{choice.name}}",
            py::overload_cast<>(&{{type_name}}::get{{choice.name}}, py::const_),
            &{{type_name}}::set{{choice.name}}
        )
        {%- endfor %}
        .def("__eq__", [](const {{type_name}}& lhs, const {{type_name}}& rhs) {return lhs == rhs;})
        .def("__repr__",
            [](const {{type_name}}& a) {
                std::stringstream ss;
                ss << a;
                return ss.str();
            }
        );

    py::enum_<{{type_name}}::Choice>({{class_name}}, "Choice")
        {%- for choice in type_info|variant.choices %}
        .value("{{choice.name}}", {{type_name}}::Choice::{{choice.name}}) {{-";" if loop.last}}
        {%- endfor %}
}
