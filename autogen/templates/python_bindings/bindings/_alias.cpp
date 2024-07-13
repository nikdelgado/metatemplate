#include <iostream>
#include <sstream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <{{path_api}}/types/{{type_name}}.h>

namespace py = pybind11;
using namespace {{ns_api}}::types;

void bind{{type_name}}(py::module_& m) {
    py::class_<{{type_name}}>(m, "{{type_name}}", py::module_local())
        .def(py::init<>())
        .def(py::init<{{type_name}}::alias_type>())
        .def("getValue", &{{type_name}}::getValue)
        .def("setValue", &{{type_name}}::setValue)
        .def("__eq__", [](const {{type_name}}& lhs, const {{type_name}}& rhs) {return lhs == rhs;})
        .def("__ne__", [](const {{type_name}}& lhs, const {{type_name}}& rhs) {return lhs != rhs;})
        .def("__lt__", [](const {{type_name}}& lhs, const {{type_name}}& rhs) {return lhs < rhs;})
        .def("__le__", [](const {{type_name}}& lhs, const {{type_name}}& rhs) {return lhs <= rhs;})
        .def("__gt__", [](const {{type_name}}& lhs, const {{type_name}}& rhs) {return lhs > rhs;})
        .def("__ge__", [](const {{type_name}}& lhs, const {{type_name}}& rhs) {return lhs >= rhs;})
        .def("__repr__",
            [](const {{type_name}}& a) {
                std::stringstream ss;
                ss << a;
                return ss.str();
            }
        );
}
