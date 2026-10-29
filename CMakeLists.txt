# Laboratoire 5 : la regulation PID, sur le simulateur de conduite.
# Compilation : CMake, comme au laboratoire 4.
#
#   cmake -B build
#   cmake --build build
#   ./build/essai_pid                                   # d'abord : eprouver votre compute(), sans simulateur
#   ./build/verif_lien ADRESSE 5000                     # au besoin : prouver la chaine
#   ./build/conduite ADRESSE 5000 0.5 0 0 essai01       # un essai : adresse, port, Kp, Ki, Kd, nom
#   ./build/conduite_taches ADRESSE 5000                # etape 5 : les gains au clavier
#   ./build/vitesse ADRESSE 5000 0.1 0 0 essai21        # etapes 4 et 7 : les gains de la vitesse
#   ./build/course ADRESSE 5000 essai41                 # etape 9 : la course finale
#
# Les executables :
#
#   essai_pid        huit verifications de votre Pid::compute().
#   verif_lien       affiche tout ce que le simulateur envoie, sans rien repondre.
#   conduite         la boucle de commande : un lancement, un essai, un fichier CSV.
#   conduite_taches  la meme boucle dans une tache de la bibliotheque du laboratoire 4.
#                    Il se construit des que votre dossier bibliotheque/ du
#                    laboratoire 4, complete, est copie a cote de src/.
#   vitesse          la boucle de vitesse, sur un scenario du simulateur.
#   course           les deux boucles et la table de gains, sur un scenario.
#
# Tout est fourni, sauf Pid::compute(), dans src/Pid.cpp.

cmake_minimum_required(VERSION 3.16)
project(conduite_pid CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Zero avertissement, avec tous les avertissements (regle 10 de The Power of 10).
set(AVERTISSEMENTS -Wall -Wextra -Wpedantic -Wshadow)

# Le lien : TcpLink (le reseau) et SimulatorLink (le simulateur).
add_library(lien STATIC src/TcpLink.cpp src/SimulatorLink.cpp)
target_include_directories(lien PUBLIC include src)
target_compile_options(lien PRIVATE ${AVERTISSEMENTS})

# La regulation : la classe Pid, la rampe de consigne, la table de gains,
# l'enregistreur de courbes.
add_library(regulation STATIC src/Pid.cpp src/SetpointRamp.cpp src/GainSchedule.cpp src/CsvRecorder.cpp)
target_include_directories(regulation PUBLIC src)
target_compile_options(regulation PRIVATE ${AVERTISSEMENTS})

# Ce qui est commun aux programmes d'essai : la ligne de commande, le compte rendu.
add_library(essai STATIC src/Trial.cpp)
target_link_libraries(essai PUBLIC lien regulation)
target_compile_options(essai PRIVATE ${AVERTISSEMENTS})

add_executable(essai_pid src/essai_pid.cpp)
target_link_libraries(essai_pid PRIVATE regulation)
target_compile_options(essai_pid PRIVATE ${AVERTISSEMENTS})

add_executable(verif_lien src/verif_lien.cpp)
target_link_libraries(verif_lien PRIVATE lien)
target_compile_options(verif_lien PRIVATE ${AVERTISSEMENTS})

add_executable(conduite src/main.cpp)
target_link_libraries(conduite PRIVATE essai)
target_compile_options(conduite PRIVATE ${AVERTISSEMENTS})

add_executable(vitesse src/vitesse.cpp)
target_link_libraries(vitesse PRIVATE essai)
target_compile_options(vitesse PRIVATE ${AVERTISSEMENTS})

add_executable(course src/course.cpp)
target_link_libraries(course PRIVATE essai)
target_compile_options(course PRIVATE ${AVERTISSEMENTS})

# Etape 5 : la boucle dans une tache, avec votre bibliotheque du laboratoire 4.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bibliotheque/Thread.cpp")
    find_package(Threads REQUIRED)
    add_library(taches STATIC
        bibliotheque/Thread.cpp bibliotheque/Mutex.cpp bibliotheque/Clock.cpp
        bibliotheque/Keyboard.cpp bibliotheque/Screen.cpp)
    target_include_directories(taches PUBLIC bibliotheque)
    target_link_libraries(taches PUBLIC Threads::Threads)

    add_executable(conduite_taches src/conduite_taches.cpp src/SharedSettings.cpp)
    target_link_libraries(conduite_taches PRIVATE essai taches)
    target_compile_options(conduite_taches PRIVATE ${AVERTISSEMENTS})
else()
    message(STATUS "conduite_taches : copiez votre dossier bibliotheque/ du laboratoire 4 a cote de src/ pour la construire")
endif()
