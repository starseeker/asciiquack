cmake_minimum_required(VERSION 3.15)

project(asciiquack
    VERSION 0.1.0
    DESCRIPTION "A C++17 implementation of the AsciiDoc text processor"
    LANGUAGES CXX C
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
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# ── Regex backend ─────────────────────────────────────────────────────────────
# Three choices (controlled by CMake options):
#
#   USE_PCRE2=ON  + USE_SYSTEM_PCRE2=ON  (default)
#       → Use system libpcre2-8 when found   (JIT enabled, fastest ~5×)
#       → Fall back to embedded vendor subset when not found (~3× faster)
#
#   USE_PCRE2=ON  + USE_SYSTEM_PCRE2=OFF
#       → Force embedded PCRE2 subset (no JIT, no external dep)
#
#   USE_PCRE2=OFF
#       → std::regex (no PCRE2 at all, slowest)
#
# RE2 is not provided as a backend; existing patterns use backreferences and
# lookaheads that RE2 intentionally omits.

option(USE_PCRE2        "Use PCRE2 regex backend (faster than std::regex)"   ON)
option(USE_SYSTEM_PCRE2 "Prefer system libpcre2-8 over embedded vendor copy" ON)

set(AQREGEX_DEFS "")
set(AQREGEX_LIBS "")
set(AQREGEX_INCS "")
set(AQREGEX_SRCS "")

if(USE_PCRE2)
    set(_pcre2_system_found FALSE)

    if(USE_SYSTEM_PCRE2)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(PCRE2 QUIET libpcre2-8)
        endif()
        if(NOT PCRE2_FOUND)
            find_library(PCRE2_LIBRARIES NAMES pcre2-8)
            find_path(PCRE2_INCLUDE_DIRS NAMES pcre2.h)
            if(PCRE2_LIBRARIES AND PCRE2_INCLUDE_DIRS)
                set(PCRE2_FOUND TRUE)
            endif()
        endif()
        if(PCRE2_FOUND)
            set(_pcre2_system_found TRUE)
        endif()
    endif()

    if(_pcre2_system_found)
        # ── System PCRE2: JIT-enabled, fastest ───────────────────────────────
        list(APPEND AQREGEX_DEFS AQREGEX_USE_PCRE2 AQREGEX_PCRE2_SYSTEM)
        list(APPEND AQREGEX_LIBS ${PCRE2_LIBRARIES})
        list(APPEND AQREGEX_INCS ${PCRE2_INCLUDE_DIRS})
        message(STATUS "  Regex backend: system PCRE2 ${PCRE2_VERSION} (JIT enabled, ~5× vs std::regex)")
    else()
        # ── Embedded PCRE2 vendor subset: no JIT, no external dependency ─────
        # Compile the 18 minimal PCRE2 .c files as part of each target.
        file(GLOB PCRE2_EMBED_SRCS "${CMAKE_SOURCE_DIR}/vendor/pcre2/*.c")
        list(APPEND AQREGEX_DEFS
            AQREGEX_USE_PCRE2
            PCRE2_STATIC
            HAVE_CONFIG_H
        )
        list(APPEND AQREGEX_SRCS ${PCRE2_EMBED_SRCS})
        list(APPEND AQREGEX_INCS "${CMAKE_SOURCE_DIR}/vendor/pcre2")
        if(USE_SYSTEM_PCRE2)
            message(STATUS "  Regex backend: embedded PCRE2 10.42 (system library not found; no JIT, ~3× vs std::regex)")
        else()
            message(STATUS "  Regex backend: embedded PCRE2 10.42 (USE_SYSTEM_PCRE2=OFF; no JIT, ~3× vs std::regex)")
        endif()
    endif()
else()
    message(STATUS "  Regex backend: std::regex (USE_PCRE2=OFF)")
endif()

# ── Helper: apply regex settings to a target ─────────────────────────────────
function(aqregex_configure target)
    if(AQREGEX_DEFS)
        target_compile_definitions(${target} PRIVATE ${AQREGEX_DEFS})
    endif()
    if(AQREGEX_LIBS)
        target_link_libraries(${target} PRIVATE ${AQREGEX_LIBS})
    endif()
    if(AQREGEX_INCS)
        target_include_directories(${target} PRIVATE ${AQREGEX_INCS})
    endif()
    if(AQREGEX_SRCS)
        target_sources(${target} PRIVATE ${AQREGEX_SRCS})
        # Suppress warnings from vendored C code; it's not our code.
        set_source_files_properties(${AQREGEX_SRCS}
            TARGET_DIRECTORY ${target}
            PROPERTIES COMPILE_FLAGS
                "-w -DPCRE2_CODE_UNIT_WIDTH=8")
    endif()
endfunction()

# ── Main executable ───────────────────────────────────────────────────────────
add_executable(asciiquack
    asciiquack.cpp
    minipdf.cpp
    parser.cpp
)
target_include_directories(asciiquack PRIVATE ${CMAKE_SOURCE_DIR})
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
    target_include_directories(asciiquack_tests PRIVATE ${CMAKE_SOURCE_DIR})
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
    target_include_directories(bench_asciiquack PRIVATE ${CMAKE_SOURCE_DIR})
    aqregex_configure(bench_asciiquack)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS asciiquack RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ── Summary ───────────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "asciiquack ${PROJECT_VERSION}")
message(STATUS "  Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard : C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler     : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Tests        : ${BUILD_TESTS}")
message(STATUS "  Benchmark    : ${BUILD_BENCHMARK}")
message(STATUS "")
