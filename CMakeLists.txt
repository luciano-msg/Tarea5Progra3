cmake_minimum_required(VERSION 3.16)
project(Tarea5_PatronesDeDiseno CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(ej1_singleton ej1-singleton/main.cpp)
target_link_libraries(ej1_singleton PRIVATE Threads::Threads)

add_executable(ej2_abstract_factory ej2-abstract-factory/main.cpp)
add_executable(ej3_composite ej3-composite/main.cpp)
add_executable(ej4_iterator ej4-iterator/main.cpp)
