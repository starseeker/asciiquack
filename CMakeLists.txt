cmake_minimum_required(VERSION 3.15)

project(asciiquack
    VERSION 0.1.0
    DESCRIPTION "A C++17 implementation of the AsciiDoc text processor"
    LANGUAGES CXX
)

# ── Language standard ────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler warnings ─────────────────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
    )
elseif(MSVC)
    add_compile_options(/W4 /WX /permissive-)
    # Silence MSVC warnings about standard library deprecations
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# ── Regex backend ─────────────────────────────────────────────────────────────
# USE_PCRE2 (default ON if PCRE2 is found) – replace std::regex with PCRE2
# for a significant throughput improvement.
#
# RE2 is intentionally not offered as a drop-in backend because the regex
# patterns in asciiquack use backreferences and lookahead assertions that RE2
# does not support.  See aqregex.hpp for details.
option(USE_PCRE2 "Use PCRE2 instead of std::regex (much faster)" ON)

set(AQREGEX_LIBS "")
set(AQREGEX_DEFS "")

if(USE_PCRE2)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PCRE2 QUIET libpcre2-8)
    endif()
    if(NOT PCRE2_FOUND)
        # Fall back to a manual search
        find_library(PCRE2_LIBRARIES NAMES pcre2-8)
        find_path(PCRE2_INCLUDE_DIRS NAMES pcre2.h)
        if(PCRE2_LIBRARIES AND PCRE2_INCLUDE_DIRS)
            set(PCRE2_FOUND TRUE)
        endif()
    endif()
    if(PCRE2_FOUND)
        list(APPEND AQREGEX_LIBS    ${PCRE2_LIBRARIES})
        list(APPEND AQREGEX_DEFS    AQREGEX_USE_PCRE2)
        message(STATUS "  Regex backend: PCRE2 (${PCRE2_VERSION})")
    else()
        message(STATUS "  Regex backend: std::regex (PCRE2 not found)")
    endif()
else()
    message(STATUS "  Regex backend: std::regex")
endif()

# ── Helper: apply regex settings to a target ─────────────────────────────────
function(aqregex_configure target)
    if(AQREGEX_DEFS)
        target_compile_definitions(${target} PRIVATE ${AQREGEX_DEFS})
    endif()
    if(AQREGEX_LIBS)
        target_link_libraries(${target} PRIVATE ${AQREGEX_LIBS})
    endif()
    if(PCRE2_INCLUDE_DIRS)
        target_include_directories(${target} PRIVATE ${PCRE2_INCLUDE_DIRS})
    endif()
endfunction()

# ── Main executable ───────────────────────────────────────────────────────────
add_executable(asciiquack
    asciiquack.cpp
    minipdf.cpp
    parser.cpp
)

target_include_directories(asciiquack PRIVATE
    ${CMAKE_SOURCE_DIR}
)
aqregex_configure(asciiquack)

# ── Tests ─────────────────────────────────────────────────────────────────────
option(BUILD_TESTS "Build the C++ test suite" ON)

if(BUILD_TESTS)
    enable_testing()

    add_executable(asciiquack_tests
        test_asciiquack.cpp
        minipdf.cpp
        parser.cpp
    )

    target_include_directories(asciiquack_tests PRIVATE
        ${CMAKE_SOURCE_DIR}
    )
    aqregex_configure(asciiquack_tests)

    add_test(NAME asciiquack_tests COMMAND asciiquack_tests)
endif()

# ── Benchmark ─────────────────────────────────────────────────────────────────
option(BUILD_BENCHMARK "Build the C++ benchmark program" ON)

if(BUILD_BENCHMARK)
    add_executable(bench_asciiquack
        benchmark/bench_asciiquack.cpp
        minipdf.cpp
        parser.cpp
    )

    target_include_directories(bench_asciiquack PRIVATE
        ${CMAKE_SOURCE_DIR}
    )
    aqregex_configure(bench_asciiquack)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS asciiquack
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ── Summary ───────────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "asciiquack ${PROJECT_VERSION}")
message(STATUS "  Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard : C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler     : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Tests        : ${BUILD_TESTS}")
message(STATUS "  Benchmark    : ${BUILD_BENCHMARK}")
message(STATUS "")
