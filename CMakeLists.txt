cmake_minimum_required(VERSION 3.16)
project(SIK_Zadanie_2_Serwer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wconversion -Werror -O2 -pthread")
#set(CMAKE_CXX_FLAGS "-pthread")

SET(Boost_USE_STATIC_LIBS FALSE)
FIND_PACKAGE(Boost REQUIRED COMPONENTS system program_options)
IF (Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF (Boost_FOUND)

SET(USED_LIBS ${Boost_SYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY})

add_executable(robots-server server.cpp server_game.cpp server_communication.cpp buffer.cpp)
TARGET_LINK_LIBRARIES(robots-server ${USED_LIBS})

add_executable(robots-client client.cpp client_game.cpp client_communication.cpp buffer.cpp)
TARGET_LINK_LIBRARIES(robots-client ${USED_LIBS})