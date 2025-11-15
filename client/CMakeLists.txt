cmake_minimum_required(VERSION 3.16)
project(codecoach_client)

set(CMAKE_CXX_STANDARD 17)

# Activar autogeneración de Qt (MOC, UIC, RCC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Buscar Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# Indicar dónde buscar los archivos .ui
set(CMAKE_AUTOUIC_SEARCH_PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

# Archivos fuente y encabezados
add_executable(codecoach_client
        src/main.cpp
        src/MainWindow.cpp
        src/MainWindow.h
        src/HttpClient.h
        src/ProblemModel.h
        ui/mainwindow.ui
)

target_include_directories(codecoach_client PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(codecoach_client PRIVATE
        Qt6::Widgets
        Qt6::Network
)
