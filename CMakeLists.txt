cmake_minimum_required(VERSION 3.20)

project(HWpilot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_add_executable(HWpilot
    src/main.cpp
    src/AppText.cpp
    src/AppText.h
    src/MainWindow.cpp
    src/MainWindow.h
    src/MainWindow/Ai.cpp
    src/MainWindow/Feedback.cpp
    src/MainWindow/MainWindowPrivate.h
    src/MainWindow/Project.cpp
    src/MainWindow/Render.cpp
    src/MainWindow/Render.h
    src/MainWindow/Ui.cpp
    src/MainWindow/Version.cpp
    src/ProjectData.cpp
    src/ProjectData.h
    src/ProjectManager/ProjectManager.cpp
    src/ProjectManager/ProjectManager.h
    src/FeedbackStore/FeedbackStore.cpp
    src/FeedbackStore/FeedbackStore.h
    src/GitService/GitService.cpp
    src/GitService/GitService.h
    src/HWFileScanner/HWFileScanner.cpp
    src/HWFileScanner/HWFileScanner.h
    src/HWpilotLLM/HWpilotLLM.cpp
    src/HWpilotLLM/HWpilotLLM.h
)

target_include_directories(HWpilot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(HWpilot PRIVATE Qt6::Widgets Qt6::Network)
