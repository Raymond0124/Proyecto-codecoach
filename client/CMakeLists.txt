cmake_minimum_required(VERSION 3.16)
project(codecoach_client LANGUAGES CXX)

# --- Configuración estándar ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Activar auto-moc, auto-uic y auto-rcc ---
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# --- Qt6 ---
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# --- Rutas donde AUTOUIC buscará los .ui ---
set(CMAKE_AUTOUIC_SEARCH_PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

# --- Archivos fuente del cliente ---
add_executable(codecoach_client
        src/main.cpp
        src/MainWindow.cpp
        src/MainWindow.h

        src/AdminWindow.cpp
        src/AdminWindow.h

        src/HttpClient.h
        src/ProblemModel.h
        src/ProblemService.cpp
        src/ProblemService.h

        ui/mainwindow.ui
        ui/adminwindow.ui
        src/analizadorsoluciones.h src/analizadorsoluciones.cpp
)

# --- Directorios de inclusión ---
target_include_directories(codecoach_client PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

# --- Enlazar librerías Qt ---
target_link_libraries(codecoach_client PRIVATE
        Qt6::Widgets
        Qt6::Network
)
