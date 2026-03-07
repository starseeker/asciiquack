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

# ── Main executable ───────────────────────────────────────────────────────────
add_executable(asciiquack
    asciiquack.cpp
    parser.cpp
)

target_include_directories(asciiquack PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# ── Tests ─────────────────────────────────────────────────────────────────────
option(BUILD_TESTS "Build the C++ test suite" ON)

if(BUILD_TESTS)
    enable_testing()

    add_executable(asciiquack_tests
        test_asciiquack.cpp
        parser.cpp
    )

    target_include_directories(asciiquack_tests PRIVATE
        ${CMAKE_SOURCE_DIR}
    )

    add_test(NAME asciiquack_tests COMMAND asciiquack_tests)
endif()

# ── Benchmark ─────────────────────────────────────────────────────────────────
option(BUILD_BENCHMARK "Build the C++ benchmark program" ON)

if(BUILD_BENCHMARK)
    add_executable(bench_asciiquack
        benchmark/bench_asciiquack.cpp
        parser.cpp
    )

    target_include_directories(bench_asciiquack PRIVATE
        ${CMAKE_SOURCE_DIR}
    )
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
