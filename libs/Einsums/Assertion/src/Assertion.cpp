//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#include <Einsums/Config.hpp>

#include <Einsums/Assert.hpp>
#include <Einsums/Debugging/Backtrace.hpp>
#include <Einsums/Version.hpp>

#include <iostream>
#include <mutex>

namespace {
einsums::detail::assertion_handler_type handler{einsums::detail::default_assertion_handler};

std::mutex handler_mutex;
} // namespace

namespace einsums::detail {

namespace {
auto get_handler() -> assertion_handler_type & {
    std::lock_guard lock(handler_mutex);

    if (handler == nullptr) {
        handler = default_assertion_handler;
    }

    return handler;
}
} // namespace

void default_assertion_handler(std::source_location const &loc, char const *expr, std::string const &msg) {
    std::cerr << complete_version() << "\n" << loc.function_name() << ":" << loc.line() << " : Assertion '" << expr << "' failed";
    if (!msg.empty()) {
        std::cerr << " (" << msg << ")\n";
    } else {
        std::cerr << "\n";
    }

    std::cerr << "\n" << util::backtrace() << "\n";

    std::exit(EXIT_FAILURE);
}

void set_assertion_handler(assertion_handler_type handler_) {
    std::lock_guard lock(handler_mutex);

    EINSUMS_LOG_DEBUG("Setting assertion handler.");

    if (handler_ == nullptr) {
        EINSUMS_LOG_TRACE("Got a null pointer for the handler. Setting it back to the default instead.");
        handler = default_assertion_handler;
    } else {
        EINSUMS_LOG_TRACE("Got an actual handler. Setting.");
        handler = handler_;
    }
}

void handle_assert(std::source_location const &loc, char const *expr, std::string const &msg) noexcept {
    std::lock_guard lock(handler_mutex);
    EINSUMS_LOG_DEBUG(complete_version());
    EINSUMS_LOG_DEBUG("{}: {}: Assertion '{}' failed", loc.function_name(), loc.line(), expr);

#ifdef EINSUMS_DEBUG
    if (!msg.empty()) {
        EINSUMS_LOG_DEBUG(msg);
    }
#endif

    if (handler == nullptr) {
        handler = default_assertion_handler;
    }

    EINSUMS_LOG_TRACE("Delegating to the assertion handler.");

    handler(loc, expr, msg);

    EINSUMS_LOG_TRACE("Finished delegating to the assertion handler.");
}

} // namespace einsums::detail
