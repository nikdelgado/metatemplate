/*
This file defines the Python module for all of the {{ns_api}} C++ classes.
*/

#include <pybind11/pybind11.h>

namespace py = pybind11;

// Forward declare all bindings functions
// These will be found during linking
{% for type_name, _type in class_map|dictsort -%}
void bind{{type_name}}(py::module_& m);
{% endfor %}

{% if not ns_utils %}
void bindClock(py::module_& m);
void bindUUID(py::module_& m);
{% endif %}

PYBIND11_MODULE(python, m)
{
    m.doc() = "Python bindings for {{ns_api}} types";

    {% if not ns_utils %}
    // std::byte for serialization
    py::class_<std::byte>(m, "StdByte", py::module_local())
        //Lambda function constructor to convert int to std::byte
        .def(py::init([](int arg) {
        return std::unique_ptr<std::byte>(new std::byte ( (std::byte)arg ));
        }));
    bindClock(m);
    bindUUID(m);
    {% endif %}

    {% for type_name, _type in class_map|dictsort -%}
    bind{{type_name}}(m);
    {% endfor %}
}