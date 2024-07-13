#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <{{path_api}}/utils/UUID.h>

namespace py = pybind11;
using namespace {{ns_api}}::utils;

void bindUUID(py::module_& m){
    py::class_<UUID>(m, "UUID", py::module_local())
        .def("__str__", [](UUID& id) {
                return boost::lexical_cast<std::string>(id); })
        .def("__repr__", [](UUID& id) {
                return boost::lexical_cast<std::string>(id); })
		.def_static(
			"fromStr", [](const std::string& str) { return boost::lexical_cast<UUID>(str); }, py::arg("id"))
		.def_static("generate", &GenerateUUID)
		.def("__eq__", [](const UUID& self, const UUID& other) {return self == other;})
		.def("__ne__", [](const UUID& self, const UUID& other) {return self != other;});

		m.def("UUIDToStr", &UUIDtoStr, py::arg("id"));
        m.def("UUIDFromStr", &UUIDfromStr, py::arg("id"));
}