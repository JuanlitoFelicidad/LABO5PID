# La bibliothèque de tâches du cours, son programme d'essai, et les trois programmes du
# laboratoire 4 : partage, ordonnancements et course.
# Construction : cmake -B build && cmake --build build
cmake_minimum_required(VERSION 3.16)
project(bibliotheque CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

find_package(Threads REQUIRED)

add_library(taches STATIC
    Thread.cpp
    Mutex.cpp
    Clock.cpp
    Keyboard.cpp
    Screen.cpp
)
target_link_libraries(taches PUBLIC Threads::Threads)
target_include_directories(taches PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(essai essai.cpp)
target_link_libraries(essai PRIVATE taches)

add_executable(ordonnancements ordonnancements.cpp)
target_link_libraries(ordonnancements PRIVATE taches)

# Étape 3 : décommentez ces deux lignes quand SharedValue.h et SharedValue.cpp existent.
add_executable(partage partage.cpp SharedValue.cpp)
target_link_libraries(partage PRIVATE taches)

# Étape 5 : décommentez ces deux lignes quand SharedTable.h et SharedTable.cpp existent.
 add_executable(course course.cpp SharedTable.cpp)
 target_link_libraries(course PRIVATE taches)
