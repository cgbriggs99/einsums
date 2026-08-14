//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/Config.hpp>

#include <Einsums/Assert.hpp>
#include <Einsums/Errors/Error.hpp>
#include <Einsums/Errors/ThrowException.hpp>
#include <Einsums/Logging.hpp>
#include <Einsums/Profile/Timer.hpp>
#include <Einsums/Runtime/InitRuntime.hpp>
#include <Einsums/Runtime/Runtime.hpp>

#include <cstdlib>

namespace einsums {

namespace detail {

static std::list<std::function<void()>> __deleters{};

void register_free_pointer(std::function<void()> f) {
    __deleters.push_back(f);
}

} // namespace detail

int finalize() {
    if (runtime_ptr()) {
        auto &rt = runtime();
        rt.call_shutdown_functions(true);
        EINSUMS_LOG_INFO("ran pre-shutdown functions");
        rt.call_shutdown_functions(false);
        EINSUMS_LOG_INFO("ran shutdown functions");

        auto &global_config = GlobalConfigMap::get_singleton();

        EINSUMS_LOG_TRACE("Creating profile report.");

        if (global_config.get_bool("profiler-report")) {
            profile::report(global_config.get_string("profiler-filename"), global_config.get_bool("profiler-append"));
        }

        EINSUMS_LOG_TRACE("Finalizing the profiler.");

        // This is the only explicit finalization routine. This is because the runtime depends on the
        // profiler. If the profiler used the normal finalization, then it would also depend on the runtime.
        // This would cause a dependency error.
        profile::finalize();

        EINSUMS_LOG_TRACE("Destroying the global runtime.");

        // this function destroys the runtime.
        rt.deinit_global_data();

        EINSUMS_LOG_TRACE("Done destroying the bulk of the library.");
    }

    EINSUMS_LOG_TRACE("Freeing hanging pointers. Printing a line for each.");
    // Free lost pointers.
    for (auto fn : detail::__deleters) {
        EINSUMS_LOG_TRACE("Freeing pointer...");
        fn();
    }

    EINSUMS_LOG_TRACE("Clearing the deleter list.");

    detail::__deleters.clear();

    EINSUMS_LOG_INFO("einsums shutdown completed");

    return EXIT_SUCCESS;
}

} // namespace einsums