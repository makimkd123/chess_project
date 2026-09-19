cmake_minimum_required(VERSION 3.20)

project(CPP_CHESS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
# Chess core tests
# --------------------------------------------------

enable_testing()

add_executable(chess_tests
    tests/TestMain.cpp
)

target_link_libraries(chess_tests
    PRIVATE
        chess_core
)

add_test(
    NAME chess_core_tests
    COMMAND chess_tests
)

# --------------------------------------------------
# Qt desktop application
# --------------------------------------------------

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(chess_desktop
    src/app/main.cpp

    src/app/MainWindow.h
    src/app/MainWindow.cpp

    src/app/ChessBoardWidget.h
    src/app/ChessBoardWidget.cpp

    src/app/theme/ChessTheme.h
    src/app/theme/ChessTheme.cpp
    src/app/theme/ThemeLoader.h
    src/app/theme/ThemeLoader.cpp
)

target_link_libraries(chess_desktop
    PRIVATE
        chess_core
        Qt6::Widgets
)
add_custom_command(
    TARGET chess_desktop
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/themes
        $<TARGET_FILE_DIR:chess_desktop>/themes
    COMMENT "Copying chess themes"
)