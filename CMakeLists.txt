cmake_minimum_required(VERSION 3.20)

# Nombre general del super-proyecto
project(codecoach_superproject)

# Por ahora solo existe el motor de evaluaciones.
# OJO: El texto dentro de las comillas debe coincidir EXACTO con
# el nombre de la carpeta: "Motor de evaluaciones"
add_subdirectory("Motor de evaluaciones")

# Más adelante, cuando creen los otros módulos, solo descomentas
# o agregas estas líneas (cuando existan esas carpetas y sus CMakeLists):

# add_subdirectory("Gestor de problemas")
# add_subdirectory("Analizador de soluciones")
