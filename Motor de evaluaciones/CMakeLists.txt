cmake_minimum_required(VERSION 3.20)
project(codecoach_motor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(codecoach_motor
        src/main.cpp
        src/MotorEvaluacion.cpp
        src/EjecutadorCodigo.cpp
        src/SandboxEjecucion.cpp
)

# Directorios donde buscar .h
target_include_directories(codecoach_motor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Librerías requeridas para Windows
if (WIN32)
    target_link_libraries(codecoach_motor PRIVATE
            psapi        # para GetProcessMemoryInfo
            ws2_32       # ⬅️ NECESARIO para cpp-httplib (WinSock)
    )
endif()
