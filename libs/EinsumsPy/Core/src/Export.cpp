//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/Config.hpp>

#include <Einsums/Config/Types.hpp>
#include <Einsums/Errors/Error.hpp>
#include <Einsums/Logging.hpp>
#include <Einsums/Profile/Section.hpp>
#include <Einsums/Runtime.hpp>

#include <exception>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <stdexcept>

namespace py = pybind11;
using namespace einsums;

bool gpu_enabled() {
#ifdef EINSUMS_COMPUTE_CODE
    return true;
#else
    return false;
#endif
}

void export_Core(py::module_ &mod) {
    mod.def("gpu_enabled", gpu_enabled)
        .def("initialize", [](std::vector<std::string> &argv) { einsums::initialize(argv); })
        .def("finalize", einsums::finalize)
        .def("log", [](int level, std::string const &str) { EINSUMS_LOG(level, str); })
        .def("log_trace", [](std::string const &str) { EINSUMS_LOG_TRACE(str); })
        .def("log_debug", [](std::string const &str) { EINSUMS_LOG_DEBUG(str); })
        .def("log_info", [](std::string const &str) { EINSUMS_LOG_INFO(str); })
        .def("log_warn", [](std::string const &str) { EINSUMS_LOG_WARN(str); })
        .def("log_error", [](std::string const &str) { EINSUMS_LOG_ERROR(str); })
        .def("log_critical", [](std::string const &str) { EINSUMS_LOG_CRITICAL(str); });

    auto config_map = py::class_<einsums::GlobalConfigMap, std::shared_ptr<einsums::GlobalConfigMap>>(mod, "GlobalConfigMap");

    config_map.def_static("get_singleton", einsums::GlobalConfigMap::get_singleton)
        .def("empty", [](GlobalConfigMap &self) { return self.empty(); })
        .def("size", [](GlobalConfigMap &self) { return self.size(); })
        .def("max_size", [](GlobalConfigMap &self) { return self.max_size(); })
        .def("get_string", [](GlobalConfigMap &self, std::string const &str) { return self.get_string(str); })
        .def("get_int", [](GlobalConfigMap &self, std::string const &str) { return self.get_int(str); })
        .def("get_double", [](GlobalConfigMap &self, std::string const &str) { return self.get_double(str); });

    auto section = py::class_<einsums::Section>(mod, "Section");

    section.def(py::init<std::string const &>())
        .def(py::init<std::string const &, bool>())
        .def(py::init<std::string const &, std::string const &>())
        .def(py::init<std::string const &, std::string const &, bool>())
        .def("end", &Section::end);
}