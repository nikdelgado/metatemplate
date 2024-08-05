#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <{{path_api}}/utils/Clock.h>

namespace py = pybind11;
using namespace {{ns_api}}::utils;

void bindClock(py::module_& m){
    py::class_<Clock>(m, "Clock", py::module_local()).def(py::init<>());

	py::class_<Duration>(m, "Duration", py::module_local())
		.def(py::init<>())
		.def(py::init<std::int64_t>())
		.def_static("zero", &Duration::zero)
		.def_static("min", &Duration::min)
		.def_static("max", &Duration::max)
		.def("count", &Duration::count)
		.def("__add__", [](const Duration& lhs, const Duration& rhs) { return lhs + rhs; })
		.def("__sub__", [](const Duration& lhs, const Duration& rhs) { return lhs - rhs; })
		.def("__iadd__", &Duration::operator+=)
		.def("__isub__", &Duration::operator-=)
		.def("__imul__", &Duration::operator*=)
		.def("__idiv__", &Duration::operator/=)
		.def("__eq__", [](const Duration& lhs, const Duration& rhs) { return lhs == rhs; })
		.def("__ne__", [](const Duration& lhs, const Duration& rhs) { return lhs != rhs; })
		.def("__lt__", [](const Duration& lhs, const Duration& rhs) { return lhs < rhs; })
		.def("__le__", [](const Duration& lhs, const Duration& rhs) { return lhs <= rhs; })
		.def("__gt__", [](const Duration& lhs, const Duration& rhs) { return lhs > rhs; })
		.def("__ge__", [](const Duration& lhs, const Duration& rhs) { return lhs >= rhs; });

	py::class_<TimePoint>(m, "TimePoint", py::module_local())
	    .def(py::init<>())
	    .def(py::init<Duration>(), "duration")
	    .def("ns_since_epoch", &TimePoint::time_since_epoch);

    m.def("DurationToDouble", &DurationToDouble, py::arg("duration"));
	m.def("DoubleToTimePoint", &DoubleToTimePoint, py::arg("time"));
}
