cmake_minimum_required(VERSION 3.20)

project(CPP_CHESS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

# --------------------------------------------------
# Chess core library
# --------------------------------------------------

add_library(chess_core STATIC
    src/models/Board.cpp

    src/logic/MoveGenerator.cpp
    src/logic/AttackDetector.cpp
    src/logic/GameState.cpp
    src/logic/Game.cpp

    src/notation/Fen.cpp
    src/notation/San.cpp
    src/notation/Pgn.cpp
)

target_include_directories(chess_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --------------------------------------------------
# Qt desktop application
# --------------------------------------------------

qt_add_executable(chess_desktop
    src/app/main.cpp

    src/app/MainWindow.h
    src/app/MainWindow.cpp

    src/app/ChessBoardWidget.h
    src/app/ChessBoardWidget.cpp
)

target_link_libraries(chess_desktop
    PRIVATE
        chess_core
        Qt6::Widgets
)