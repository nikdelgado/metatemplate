#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <{{path_api}}/types/{{type_name}}.h>

namespace py = pybind11;
using namespace {{ns_api}}::types;

void bind{{type_name}}(py::module_& m)
{
    py::enum_<{{type_name}}>(m, "{{type_name}}", py::module_local())
        {%- for attr in type_info.attrs %}
        .value("{{attr|enum.name}}", {{type_name}}::{{attr|enum.name}})
        {%- endfor %}
        .export_values();

}
