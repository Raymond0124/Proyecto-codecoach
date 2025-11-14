cmake_minimum_required(VERSION 3.15)
project(codecoach_motor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/W4 /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party
)

add_executable(codecoach_motor
        src/main.cpp
        src/MotorEvaluacion.cpp
        src/EjecutadorCodigo.cpp
        src/SandboxEjecucion.cpp
)
