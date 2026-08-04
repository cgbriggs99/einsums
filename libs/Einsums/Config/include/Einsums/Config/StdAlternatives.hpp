//----------------------------------------------------------------------------------------------
// Copyright (c) The Einsums Developers. All rights reserved.
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.
//----------------------------------------------------------------------------------------------

#pragma once

/*
 * Windows doesn't like certain library functions. We want to use library functions. This header provides wrappers for library functions
 * that don't throw errors on Windows.
 */

#include <Einsums/Config/CompilerSpecific.hpp>
#include <Einsums/Config/ExportDefinitions.hpp>

#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <type_traits>

#ifdef EINSUMS_WINDOWS
#    include <process.h>
#    include <stdlib.h>
#else
#    include <unistd.h>
#endif

namespace einsums {

#ifndef EINSUMS_WINDOWS
using errno_t      = int;
using safe_compare = int (*)(void *context, void const *key, void const *datum);
#else
using safe_compare = int(__cdecl *)(void *context, void const *key, void const *datum);
#endif

// Just going in alphabetical order, looking for things with an _s in the Windows C runtime.

namespace detail {
[[nodiscard]] EINSUMS_EXPORT errno_t validate_timestruct(struct ::std::tm const *time_ptr) noexcept;

// The actual asctime is really bad. Write our own since it's probably going to be deprecated and it's not too hard.
// Dealing with locales would be harder.
// Also, this does no validation. Validation is done by the callers.
[[nodiscard]] EINSUMS_EXPORT errno_t asctime_convert(char *out_buffer, struct ::std::tm const *time_ptr);

} // namespace detail

// Yes, these are nodiscard. Everyone always forgets that these functions have return values that need to be checked.
[[nodiscard]] EINSUMS_EXPORT int fprintf_s(std::FILE *fp, char const *format, ...);

[[nodiscard]] EINSUMS_EXPORT int fscanf_s(std::FILE *fp, char const *format, ...);

[[nodiscard]] EINSUMS_EXPORT int printf_s(char const *format, ...);

[[nodiscard]] EINSUMS_EXPORT int scanf_s(char const *format, ...);

// No sprintf. We shouldn't be using it anyways. No snprintf since it's already safe. The _snprintf_s function is considered optional, it
// seems. This is due to the fact that _snprintf isn't standards conformant. Standards-conformant snprintf is essentially the same as
// _snprintf_s.

[[nodiscard]] EINSUMS_EXPORT int sscanf_s(char const *buffer, char const *format, ...);

#ifdef EINSUMS_WINDOWS
[[nodiscard]] inline errno_t asctime_s(char *out_buffer, ::std::size_t number_of_elements, struct ::std::tm const *time_ptr) noexcept {
    return ::asctime_s(out_buffer, number_of_elements, time_ptr);
}

[[nodiscard]] inline void *bsearch_s(void const *key, void const *base, ::std::size_t number, ::std::size_t width, safe_compare compare,
                                     void *context) {
    return ::bsearch_s(key, base, number, width, compare, context);
}

[[nodiscard]] inline errno_t clearerr_s(::std::FILE *stream) {
    return ::clearerr_s(stream);
}

[[nodiscard]] inline errno_t ctime_s(char *buffer, ::std::size_t size, ::std::time_t const *source_time) {
    return ::ctime_s(buffer, size, source_time);
}

[[nodiscard]] inline errno_t fopen_s(std::FILE **fp, char const *filename, char const *mode) {
    return ::fopen_s(fp, filename, mode);
}

[[nodiscard]] inline std::size_t fread_s(void *buffer, std::size_t buffer_size, std::size_t element_size, std::size_t count,
                                         std::FILE *fp) {
    return ::fread_s(buffer, buffer_size, element_size, count, fp);
}

[[nodiscard]] inline errno_t freopen_s(std::FILE **stream, char const *file_name, char const *mode, std::FILE *old_fp) {
    return ::freopen_s(stream, file_name, mode, old_fp);
}

[[nodiscard]] inline errno_t getenv_s(std::size_t *needed_size, char *buffer, std::size_t buffer_size, char const *var_name) {
    return ::getenv_s(needed_size, buffer, buffer_size, var_name);
}

// The Windows standard says that buffer size should be size_t. The C++ standard says that fgets takes int.
// Prefer int in this case.
[[nodiscard]] inline char *gets_s(char *buffer, int buffer_size) {
    return ::gets_s(buffer, buffer_size);
}

[[nodiscard]] inline errno_t putenv_s(char const *var_name, char const *value) {
    return ::_putenv_s(var_name, value);
}

[[nodiscard]] inline int getpid() {
    return ::_getpid();
}

[[nodiscard]] inline errno_t gmtime_s(struct std::tm *tm_out, std::time_t const *time) {
    return ::gmtime_s(tm_out, time);
}

[[nodiscard]] inline errno_t localtime_s(struct std::tm *tm_out, std::time_t const *time) {
    return ::localtime_s(tm_out, time);
}

[[nodiscard]] inline errno_t mbsrtowcs_s(std::size_t *ret_val, wchar_t *dst, std::size_t dest_size, char const **src, std::size_t count,
                                         std::mbstate_t *state) {
    return ::mbsrtowcs_s(ret_val, dst, dest_size, src, count, state);
}

[[nodiscard]] inline errno_t memcpy_s(void *dest, std::size_t dest_size, void const *src, std::size_t count) {
    return ::memcpy_s(dest, dest_size, src, count);
}

[[nodiscard]] inline errno_t memmove_s(void *dest, std::size_t dest_size, void const *src, std::size_t count) {
    return ::memmove_s(dest, dest_size, src, count);
}

inline void qsort_s(void *base, std::size_t elements, std::size_t width, safe_compare compare, void *context) {
    return ::qsort_s(base, elements, width, compare, context);
}

[[nodiscard]] inline errno_t strcat_s(char *dest, std::size_t dest_size, char const *src) {
    return ::strcat_s(dest, dest_size, src);
}

[[nodiscard]] inline errno_t strcpy_s(char *dest, std::size_t dest_size, char const *src) {
    return ::strcpy_s(dest, dest_size, src);
}

[[nodiscard]] inline errno_t strerror_s(char *buffer, std::size_t buff_size, errno_t error_code) {
    return ::strerror_s(buffer, buff_size, error_code);
}

[[nodiscard]] inline errno_t strncat_s(char *dest, std::size_t dest_size, char const *src, std::size_t count) {
    return ::strncat_s(dest, dest_size, src, count);
}

[[nodiscard]] inline errno_t strncpy_s(char *dest, std::size_t dest_size, char const *src, std::size_t count) {
    return ::strncpy_s(dest, dest_size, src, count);
}

[[nodiscard]] inline char *strtok_s(char *str, char const *delimiters, char **context) {
    return ::strtok_s(str, delimiters, context);
}

[[nodiscard]] inline errno_t tmpfile_s(std::FILE **fp) {
    return ::tmpfile_s(fp);
}

[[nodiscard]] inline int vfprintf_s(std::FILE *fp, char const *format, std::va_list args) {
    return ::vfprintf_s(fp, format, args);
}

[[nodiscard]] inline int vfscanf_s(std::FILE *fp, char const *format, std::va_list args) {
    return ::vfscanf_s(fp, format, args);
}

[[nodiscard]] inline int vprintf_s(char const *format, std::va_list args) {
    return ::vprintf_s(format, args);
}

[[nodiscard]] inline int vscanf_s(char const *format, std::va_list args) {
    return ::vscanf_s(format, args);
}

[[nodiscard]] inline int vsscanf_s(char const *buffer, char const *format, std::va_list args) {
    return ::vsscanf_s(buffer, format, args);
}

#else
[[nodiscard]] EINSUMS_EXPORT errno_t asctime_s(char *out_buffer, ::std::size_t number_of_elements, struct ::std::tm const *time_ptr);

[[nodiscard]] EINSUMS_EXPORT void *bsearch_s(void const *key, void const *base, ::std::size_t number, ::std::size_t width,
                                             safe_compare compare, void *context);

[[nodiscard]] EINSUMS_EXPORT errno_t clearerr_s(::std::FILE *stream);

[[nodiscard]] inline errno_t ctime_s(char *out_buffer, ::std::size_t size, ::std::time_t const *source_time) {
    return asctime_s(out_buffer, size, std::localtime(source_time));
}

[[nodiscard]] EINSUMS_EXPORT errno_t fopen_s(std::FILE **fp, char const *filename, char const *mode);

[[nodiscard]] EINSUMS_EXPORT std::size_t fread_s(void *buffer, std::size_t buffer_size, std::size_t element_size, std::size_t count,
                                                 std::FILE *fp);

[[nodiscard]] EINSUMS_EXPORT errno_t freopen_s(std::FILE **fp, char const *filename, char const *mode, std::FILE *old_fp);

[[nodiscard]] EINSUMS_EXPORT errno_t getenv_s(std::size_t *needed_size, char *buffer, std::size_t buffer_size, char const *var_name);

[[nodiscard]] inline char *gets_s(char *buffer, int buffer_size) {
    return std::fgets(buffer, buffer_size, stdin);
}

[[nodiscard]] inline int getpid() {
    return ::getpid();
}

[[nodiscard]] EINSUMS_EXPORT errno_t gmtime_s(struct std::tm *tm_out, std::time_t const *time);

[[nodiscard]] EINSUMS_EXPORT errno_t localtime_s(struct std::tm *tm_out, std::time_t const *time);

[[nodiscard]] EINSUMS_EXPORT errno_t memcpy_s(void *dest, std::size_t dest_size, void const *src, std::size_t count);

[[nodiscard]] EINSUMS_EXPORT errno_t memmove_s(void *dest, std::size_t dest_size, void const *src, std::size_t count);

EINSUMS_EXPORT void qsort_s(void *base, std::size_t elements, std::size_t width, safe_compare compare, void *context);

[[nodiscard]] EINSUMS_EXPORT errno_t strcat_s(char *dest, std::size_t dest_size, char const *src);

[[nodiscard]] EINSUMS_EXPORT errno_t strcpy_s(char *dest, std::size_t dest_size, char const *src);

[[nodiscard]] EINSUMS_EXPORT errno_t strerror_s(char *buffer, std::size_t buff_size, errno_t error_code);

[[nodiscard]] EINSUMS_EXPORT errno_t strncat_s(char *dest, std::size_t dest_size, char const *src, std::size_t count);

[[nodiscard]] EINSUMS_EXPORT errno_t strcpy_s(char *dest, std::size_t dest_size, char const *src, std::size_t count);

[[nodiscard]] EINSUMS_EXPORT char *strtok_s(char *str, char const *delimiters, char **context);

[[nodiscard]] EINSUMS_EXPORT errno_t tmpfile_s(std::FILE **fp);

[[nodiscard]] inline int vfprintf_s(std::FILE *fp, char const *format, std::va_list args) {
    return std::vfprintf(fp, format, args);
}

[[nodiscard]] inline int vfscanf_s(std::FILE *fp, char const *format, std::va_list args) {
    return std::vfscanf(fp, format, args);
}

[[nodiscard]] inline int vprintf_s(char const *format, std::va_list args) {
    return std::vprintf(format, args);
}

[[nodiscard]] inline int vscanf_s(char const *format, std::va_list args) {
    return std::vscanf(format, args);
}

[[nodiscard]] inline int vsscanf_s(char const *buffer, char const *format, std::va_list args) {
    return std::vsscanf(buffer, format, args);
}

#endif

} // namespace einsums
