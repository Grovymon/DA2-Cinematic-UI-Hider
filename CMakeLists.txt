cmake_minimum_required(VERSION 3.20)
project(DA2CinematicUIHider VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "DA2 Cinematic UI Hider is a Windows-only mod.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CORE_SOURCES
    src/core/hold_state.cpp
    src/core/key_chord.cpp
    src/core/da2_bindings.cpp
)

add_library(DA2CinematicUIHider SHARED
    src/mod.cpp
    ${CORE_SOURCES}
)
target_include_directories(DA2CinematicUIHider PRIVATE src)
target_link_libraries(DA2CinematicUIHider PRIVATE user32 shell32)
set_target_properties(DA2CinematicUIHider PROPERTIES
    PREFIX ""
    SUFFIX ".asi"
    OUTPUT_NAME "DA2CinematicUIHider"
)

if(MSVC)
    target_compile_options(DA2CinematicUIHider PRIVATE /W4 /permissive- /utf-8)
    set_property(TARGET DA2CinematicUIHider PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

enable_testing()
add_executable(DA2CinematicUIHiderTests
    tests/test_main.cpp
    ${CORE_SOURCES}
)
target_include_directories(DA2CinematicUIHiderTests PRIVATE src)
if(MSVC)
    target_compile_options(DA2CinematicUIHiderTests PRIVATE /W4 /permissive- /utf-8)
    set_property(TARGET DA2CinematicUIHiderTests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
add_test(NAME deterministic_hold_and_binding_tests COMMAND DA2CinematicUIHiderTests)
