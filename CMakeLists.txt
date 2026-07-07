cmake_minimum_required(VERSION 3.20)

project(CPP_CHESS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(MyProject
    src/main/main.cpp
    src/models/Board.cpp
    src/logic/MoveGenerator.cpp
    src/logic/AttackDetector.cpp
    src/logic/GameState.cpp
    src/logic/Game.cpp
    src/notation/Fen.cpp
    src/notation/San.cpp
    src/notation/Pgn.cpp
)

target_include_directories(MyProject PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)