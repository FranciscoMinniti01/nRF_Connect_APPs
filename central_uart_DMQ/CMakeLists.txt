# CMakeLists INIT

cmake_minimum_required(VERSION 3.20.0)                  # Especifica la versión mínima de CMake requerida para este proyecto.

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})   # Busca y configura Zephyr. El directorio base de Zephyr es determinado por la variable de entorno ZEPHYR_BASE.
project(central_uart_DMQ)                               # Declara el nombre del proyecto.

# NORDIC SDK APP START

file(GLOB app_sources "src/*.c")                        # file(GLOB) genera una lista con todos los archivos fuente dentro de la carpeta src/ en app_sources

target_sources(app PRIVATE ${app_sources})              # Asigna todos los archivos fuente encontrados a la aplicación.

# NORDIC SDK APP END

# CMakeLists END 
