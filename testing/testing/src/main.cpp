//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//--------------------------------------------------------------------------------------------

#include <Einsums/Debugging/Backtrace.hpp>
#include <Einsums/Logging.hpp>
#include <Einsums/Profile.hpp>
#include <Einsums/Runtime.hpp>
#include <Einsums/Runtime/ShutdownFunction.hpp>
#include <Einsums/Utilities/Random.hpp>
#include <Einsums/Version.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/internal/catch_context.hpp>
#include <csignal>
#include <cstdio>
#include <functional>
#include <iostream>
#include <thread>

#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>

#ifdef EINSUMS_HAVE_BACKTRACES
#    include <cpptrace/cpptrace.hpp>
#    include <cpptrace/io.hpp>
#endif

using namespace std::chrono_literals;

extern "C" [[noreturn]] void signal_handler(int signal) {
    switch (signal) {
    case SIGABRT:
        std::fprintf(stderr, "SIGABRT called.\n");
        break;
    case SIGFPE:
        std::fprintf(stderr, "SIGFPE called.\n");
        break;
    case SIGILL:
        std::fprintf(stderr, "SIGILL called.\n");
        break;
    case SIGINT:
        std::fprintf(stderr, "SIGINT called.\n");
        break;
    case SIGSEGV:
        std::fprintf(stderr, "SIGSEGV called.\n");
        break;
    case SIGTERM:
        std::fprintf(stderr, "SIGABRT called.\n");
        break;
#ifdef SIGALRM
    case SIGALRM:
        std::fprintf(stderr, "SIGALRM called.\n");
        break;
#endif
#ifdef SIGBUS
    case SIGBUS:
        std::fprintf(stderr, "SIGBUS called.\n");
        break;
#endif
#ifdef SIGHUP
    case SIGHUP:
        std::fprintf(stderr, "SIGHUP called.\n");
        break;
#endif
#ifdef SIGKILL
    case SIGKILL:
        std::fprintf(stderr, "SIGKILL called.\n");
        break;
#endif
#ifdef SIGPIPE
    case SIGPIPE:
        std::fprintf(stderr, "SIGPIPE called.\n");
        break;
#endif
#ifdef SIGQUIT
    case SIGQUIT:
        std::fprintf(stderr, "SIGQUIT called.\n");
        break;
#endif
#ifdef SIGTRAP
    case SIGTRAP:
        std::fprintf(stderr, "SIGTRAP called.\n");
        break;
#endif
    default:
        std::fprintf(stderr, "Unknown signal called.\n");
    }

#ifdef EINSUMS_HAVE_BACKTRACES
    einsums::util::print_backtrace(std::cout);
#endif

    std::_Exit(1);
}

int einsums_main(int argc, char *const *const argv) {
    int result = 0;
#pragma omp parallel
    {
#pragma omp single
        {
            Catch::Session session;
            session.applyCommandLine(argc, argv);

            Catch::StringMaker<float>::precision  = std::numeric_limits<float>::digits10;
            Catch::StringMaker<double>::precision = std::numeric_limits<double>::digits10;
            auto seed                             = session.config().rngSeed();

#pragma omp parallel
            {
                einsums::random_engine().seed(seed);
            }

            {
                LabeledSection("einsums_main");
                result = session.run();
            }

            EINSUMS_LOG_INFO("Finalizing Einsums");
            einsums::finalize();

            std::printf("Done.\n");
        }
    }

    return result;
}

int main(int argc, char **argv) {

#ifdef EINSUMS_HAVE_BACKTRACES
    cpptrace::register_terminate_handler();
#endif

    std::signal(SIGABRT, signal_handler);
    std::signal(SIGFPE, signal_handler);
    std::signal(SIGILL, signal_handler);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGSEGV, signal_handler);
    std::signal(SIGTERM, signal_handler);

#ifdef SIGHUP
    std::signal(SIGHUP, signal_handler);
#endif

#ifdef SIGQUIT
    std::signal(SIGQUIT, signal_handler);
#endif

#ifdef SIGTRAP
    std::signal(SIGTRAP, signal_handler);
#endif

#ifdef SIGKILL
    std::signal(SIGKILL, signal_handler);
#endif

#ifdef SIGPIPE
    std::signal(SIGPIPE, signal_handler);
#endif

#ifdef SIGALRM
    std::signal(SIGALRM, signal_handler);
#endif

#ifdef SIGBUS
    std::signal(SIGBUS, signal_handler);
#endif

    std::signal(0, signal_handler);

    return einsums::start(einsums_main, argc, argv);
}
