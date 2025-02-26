# Copyright (C) 2020  Matthew "strager" Glazar
# See end of file for extended copyright information.

cmake_minimum_required(VERSION 3.10)
include(GNUInstallDirs)
include(QuickLintJSCompiler)
include(QuickLintJSCopyright)
include(QuickLintJSTarget)

option(
  QUICK_LINT_JS_FEATURE_VECTOR_PROFILING
  "Enable the QLJS_DUMP_VECTORS option at run-time"
  FALSE
)

set(NEED_THREADS TRUE)

if (NEED_THREADS)
  find_package(Threads REQUIRED)
endif ()

if (QUICK_LINT_JS_ENABLE_CLI)
  set(EXTRA_SOURCES)
  if (WIN32)
    list(APPEND EXTRA_SOURCES quick-lint-js/cli/quick-lint-js.rc)
    # TODO(strager): Add a dependency on dusty-icon.ico which is referenced by
    # quick-lint-js.rc.
  endif ()
  quick_lint_js_add_executable(
    quick-lint-js
    quick-lint-js/cli/main.cpp
    ${EXTRA_SOURCES}
  )
  set_target_properties(quick-lint-js PROPERTIES RUNTIME_OUTPUT_DIRECTORY ..)
  target_link_libraries(quick-lint-js PRIVATE quick-lint-js-lib)
  quick_lint_js_enable_windows_unicode(quick-lint-js)

  # TODO(strager): Use the default DESTINATION for CMake versions which have a
  # default. (3.18.0 has a default; 3.12.4 doesn't have a default.)
  install(TARGETS quick-lint-js RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif ()

quick_lint_js_add_library(
  quick-lint-js-lib
  quick-lint-js/assert.cpp
  quick-lint-js/assert.h
  quick-lint-js/boost.cpp
  quick-lint-js/c-api-diag-reporter.cpp
  quick-lint-js/c-api-diag-reporter.h
  quick-lint-js/c-api.cpp
  quick-lint-js/c-api.h
  quick-lint-js/cli/arg-parser.cpp
  quick-lint-js/cli/arg-parser.h
  quick-lint-js/cli/cli-location.cpp
  quick-lint-js/cli/cli-location.h
  quick-lint-js/cli/emacs-lisp-diag-reporter.cpp
  quick-lint-js/cli/emacs-lisp-diag-reporter.h
  quick-lint-js/cli/emacs-location.cpp
  quick-lint-js/cli/emacs-location.h
  quick-lint-js/cli/options.cpp
  quick-lint-js/cli/options.h
  quick-lint-js/cli/text-diag-reporter.cpp
  quick-lint-js/cli/text-diag-reporter.h
  quick-lint-js/cli/vim-location.cpp
  quick-lint-js/cli/vim-location.h
  quick-lint-js/cli/vim-qflist-json-diag-reporter.cpp
  quick-lint-js/cli/vim-qflist-json-diag-reporter.h
  quick-lint-js/configuration/basic-configuration-filesystem.cpp
  quick-lint-js/configuration/basic-configuration-filesystem.h
  quick-lint-js/configuration/change-detecting-filesystem-inotify.cpp
  quick-lint-js/configuration/change-detecting-filesystem-kqueue.cpp
  quick-lint-js/configuration/change-detecting-filesystem-win32.cpp
  quick-lint-js/configuration/change-detecting-filesystem.cpp
  quick-lint-js/configuration/change-detecting-filesystem.h
  quick-lint-js/configuration/configuration-loader.cpp
  quick-lint-js/configuration/configuration-loader.h
  quick-lint-js/configuration/configuration.cpp
  quick-lint-js/configuration/configuration.h
  quick-lint-js/container/allocator.h
  quick-lint-js/container/async-byte-queue.cpp
  quick-lint-js/container/async-byte-queue.h
  quick-lint-js/container/byte-buffer.cpp
  quick-lint-js/container/byte-buffer.h
  quick-lint-js/container/concat.h
  quick-lint-js/container/hash-map.h
  quick-lint-js/container/hash-set.h
  quick-lint-js/container/hash.h
  quick-lint-js/container/heap-function.h
  quick-lint-js/container/linked-bump-allocator.h
  quick-lint-js/container/linked-vector.h
  quick-lint-js/container/optional.h
  quick-lint-js/container/padded-string.cpp
  quick-lint-js/container/padded-string.h
  quick-lint-js/container/result.h
  quick-lint-js/container/vector.h
  quick-lint-js/document.cpp
  quick-lint-js/document.h
  quick-lint-js/fe/buffering-diag-reporter.cpp
  quick-lint-js/fe/buffering-diag-reporter.h
  quick-lint-js/fe/buffering-visitor.cpp
  quick-lint-js/fe/buffering-visitor.h
  quick-lint-js/fe/diag-code-list.cpp
  quick-lint-js/fe/diag-code-list.h
  quick-lint-js/fe/diag-reporter.cpp
  quick-lint-js/fe/diag-reporter.h
  quick-lint-js/fe/diagnostic-formatter.cpp
  quick-lint-js/fe/diagnostic-formatter.h
  quick-lint-js/fe/diagnostic-types.h
  quick-lint-js/fe/diagnostic.cpp
  quick-lint-js/fe/diagnostic.h
  quick-lint-js/fe/expression.h
  quick-lint-js/fe/global-declared-variable-set.cpp
  quick-lint-js/fe/global-declared-variable-set.h
  quick-lint-js/fe/global-variables-browser-generated.cpp
  quick-lint-js/fe/global-variables.cpp
  quick-lint-js/fe/global-variables.h
  quick-lint-js/fe/identifier.h
  quick-lint-js/fe/jsx.cpp
  quick-lint-js/fe/jsx.h
  quick-lint-js/fe/language.h
  quick-lint-js/fe/lex-keyword-generated.cpp
  quick-lint-js/fe/lex-unicode-generated.cpp
  quick-lint-js/fe/lex.cpp
  quick-lint-js/fe/lex.h
  quick-lint-js/fe/linter.cpp
  quick-lint-js/fe/linter.h
  quick-lint-js/fe/multi-parse-visitor.h
  quick-lint-js/fe/null-visitor.cpp
  quick-lint-js/fe/null-visitor.h
  quick-lint-js/fe/parse-class.cpp
  quick-lint-js/fe/parse-expression.cpp
  quick-lint-js/fe/parse-statement.cpp
  quick-lint-js/fe/parse-type.cpp
  quick-lint-js/fe/parse-visitor.h
  quick-lint-js/fe/parse.cpp
  quick-lint-js/fe/parse.h
  quick-lint-js/fe/reported-diag-statistics.h
  quick-lint-js/fe/source-code-span.cpp
  quick-lint-js/fe/source-code-span.h
  quick-lint-js/fe/variable-analyzer.cpp
  quick-lint-js/fe/variable-analyzer.h
  quick-lint-js/feature.h
  quick-lint-js/i18n/locale.cpp
  quick-lint-js/i18n/locale.h
  quick-lint-js/i18n/translation-table-generated.cpp
  quick-lint-js/i18n/translation-table-generated.h
  quick-lint-js/i18n/translation-table.h
  quick-lint-js/i18n/translation.cpp
  quick-lint-js/i18n/translation.h
  quick-lint-js/io/event-loop.h
  quick-lint-js/io/file-canonical.cpp
  quick-lint-js/io/file-canonical.h
  quick-lint-js/io/file-handle.cpp
  quick-lint-js/io/file-handle.h
  quick-lint-js/io/file-path.cpp
  quick-lint-js/io/file-path.h
  quick-lint-js/io/file.cpp
  quick-lint-js/io/file.h
  quick-lint-js/io/output-stream.cpp
  quick-lint-js/io/output-stream.h
  quick-lint-js/io/pipe-writer.cpp
  quick-lint-js/io/pipe-writer.h
  quick-lint-js/io/pipe.cpp
  quick-lint-js/io/pipe.h
  quick-lint-js/io/temporary-directory.cpp
  quick-lint-js/io/temporary-directory.h
  quick-lint-js/json.cpp
  quick-lint-js/json.h
  quick-lint-js/logging/log.h
  quick-lint-js/logging/logger.cpp
  quick-lint-js/logging/logger.h
  quick-lint-js/logging/trace-flusher.cpp
  quick-lint-js/logging/trace-flusher.h
  quick-lint-js/logging/trace-metadata.cpp
  quick-lint-js/logging/trace-metadata.h
  quick-lint-js/logging/trace-stream-reader.cpp
  quick-lint-js/logging/trace-writer.cpp
  quick-lint-js/logging/trace-writer.h
  quick-lint-js/lsp/lsp-diag-reporter.cpp
  quick-lint-js/lsp/lsp-diag-reporter.h
  quick-lint-js/lsp/lsp-endpoint.cpp
  quick-lint-js/lsp/lsp-endpoint.h
  quick-lint-js/lsp/lsp-location.cpp
  quick-lint-js/lsp/lsp-location.h
  quick-lint-js/lsp/lsp-message-parser.cpp
  quick-lint-js/lsp/lsp-message-parser.h
  quick-lint-js/lsp/lsp-pipe-writer.cpp
  quick-lint-js/lsp/lsp-pipe-writer.h
  quick-lint-js/lsp/lsp-server.cpp
  quick-lint-js/lsp/lsp-server.h
  quick-lint-js/lsp/lsp-uri.cpp
  quick-lint-js/lsp/lsp-uri.h
  quick-lint-js/lsp/lsp-workspace-configuration.cpp
  quick-lint-js/lsp/lsp-workspace-configuration.h
  quick-lint-js/port/attribute.h
  quick-lint-js/port/bit.h
  quick-lint-js/port/char8.cpp
  quick-lint-js/port/char8.h
  quick-lint-js/port/consteval.h
  quick-lint-js/port/crash.h
  quick-lint-js/port/have.h
  quick-lint-js/port/in-range.h
  quick-lint-js/port/integer.cpp
  quick-lint-js/port/integer.h
  quick-lint-js/port/limits.h
  quick-lint-js/port/simd.h
  quick-lint-js/port/source-location.h
  quick-lint-js/port/thread.cpp
  quick-lint-js/port/thread.h
  quick-lint-js/port/type-traits.h
  quick-lint-js/port/unreachable.h
  quick-lint-js/port/warning.h
  quick-lint-js/port/windows-error.cpp
  quick-lint-js/port/windows-error.h
  quick-lint-js/port/windows.h
  quick-lint-js/simdjson.cpp
  quick-lint-js/simdjson.h
  quick-lint-js/util/binary-reader.h
  quick-lint-js/util/binary-writer.h
  quick-lint-js/util/cpp.h
  quick-lint-js/util/math-overflow.h
  quick-lint-js/util/narrow-cast.h
  quick-lint-js/util/pointer.h
  quick-lint-js/util/try-catch-stack.h
  quick-lint-js/util/utf-16.cpp
  quick-lint-js/util/utf-16.h
  quick-lint-js/util/utf-8.cpp
  quick-lint-js/util/utf-8.h
  quick-lint-js/version.h
  quick-lint-js/web-demo-location.cpp
  quick-lint-js/web-demo-location.h
)
target_compile_definitions(
  quick-lint-js-lib
  PUBLIC
  "QUICK_LINT_JS_VERSION=${QUICK_LINT_JS_VERSION}"
)
target_include_directories(quick-lint-js-lib PUBLIC .)
target_link_libraries(quick-lint-js-lib PUBLIC boost simdjson)
if (WIN32)
  target_link_libraries(quick-lint-js-lib PUBLIC pathcch)
endif ()
if (NEED_THREADS)
  target_link_libraries(quick-lint-js-lib PRIVATE Threads::Threads)
endif ()
if (QUICK_LINT_JS_PRECOMPILE_HEADERS)
  target_precompile_headers(
    quick-lint-js-lib
    PUBLIC
    <boost/container/pmr/polymorphic_allocator.hpp>
    <cmath>
    <cstring>
    <simdjson.h>
    <string>
  )
endif ()

if (QUICK_LINT_JS_FEATURE_VECTOR_PROFILING)
  target_compile_definitions(
    quick-lint-js-lib
    PUBLIC
    QLJS_FEATURE_VECTOR_PROFILING=1
  )
else ()
  target_compile_definitions(
    quick-lint-js-lib
    PUBLIC
    QLJS_FEATURE_VECTOR_PROFILING=0
  )
endif ()
if (QUICK_LINT_JS_FEATURE_VECTOR_PROFILING)
  target_link_libraries(quick-lint-js-lib PRIVATE quick-lint-js-lib-debug)
endif ()

quick_lint_js_have_charconv(QUICK_LINT_JS_HAVE_CHARCONV)
if (NOT QUICK_LINT_JS_HAVE_CHARCONV)
  target_compile_definitions(
    quick-lint-js-lib
    PUBLIC
    QLJS_HAVE_CHARCONV_HEADER=0
  )
endif ()

# quick-lint-js-lib-debug should be dead-code stripped from production builds.
# It is a separate library from quick-lint-js-lib to increase the chance that
# the linker drops the code entirely.
#
# Code in quick-lint-js-lib-debug can be used from tests, but should not be used
# in production code.
quick_lint_js_add_library(
  quick-lint-js-lib-debug
  quick-lint-js/cli/cli-location-debug.cpp
  quick-lint-js/cli/emacs-location-debug.cpp
  quick-lint-js/cli/vim-location-debug.cpp
  quick-lint-js/container/padded-string-debug.cpp
  quick-lint-js/container/vector-profiler-debug.cpp
  quick-lint-js/fe/diag-debug.cpp
  quick-lint-js/fe/language-debug.cpp
  quick-lint-js/fe/lex-debug.cpp
  quick-lint-js/lsp/lsp-location-debug.cpp
  quick-lint-js/port/char8-debug.cpp
)
target_link_libraries(quick-lint-js-lib-debug PUBLIC boost quick-lint-js-lib)

# HACK(strager): Work around GCC compiler bug. GCC 9.3.0 miscompiles a call to
# strncmp, causing the length given to strncmp to be incorrect. (Perhaps we are
# invoking undefined behaviour though with our string table offset pointer
# arithmetic? Unsure.) The bug appears to be fixed in GCC 10.1.0.
#
# Possibly related: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92765
#
# To work around the bug, apply -fno-builtin to the target. If we apply it only
# to the source file, the flag seems to be ignored when using precompiled
# headers.
if (
  CMAKE_CXX_COMPILER_ID STREQUAL GNU
  AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.1.0
)
  target_compile_options(quick-lint-js-lib PRIVATE -fno-builtin)
endif ()

if (QUICK_LINT_JS_ENABLE_CLI)
  quick_lint_js_collect_copyright(
    quick-lint-js-licenses
    TARGET quick-lint-js
    OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/quick-lint-js-licenses.txt"
  )
  if (TARGET quick-lint-js-licenses)
    install(
      FILES "${CMAKE_CURRENT_BINARY_DIR}/quick-lint-js-licenses.txt"
      DESTINATION "${QUICK_LINT_JS_INSTALL_LICENSES_DIR}"
      RENAME copyright
    )
  endif ()
endif ()

quick_lint_js_add_library(
  quick-lint-js-boost-json
  quick-lint-js/boost-json.cpp
  quick-lint-js/boost-json.h
)
target_include_directories(quick-lint-js-boost-json PUBLIC .)
target_link_libraries(
  quick-lint-js-boost-json
  PUBLIC
  boost_json
  quick-lint-js-lib
)
target_compile_definitions(
  quick-lint-js-boost-json
  PRIVATE
  QLJS_BUILDING_QLJS_BOOST_JSON
)

# quick-lint-js finds bugs in JavaScript programs.
# Copyright (C) 2020  Matthew "strager" Glazar
#
# This file is part of quick-lint-js.
#
# quick-lint-js is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# quick-lint-js is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with quick-lint-js.  If not, see <https://www.gnu.org/licenses/>.
