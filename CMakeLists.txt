cmake_minimum_required(VERSION 3.20)

project(HWpilot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_add_executable(HWpilot
    main.cpp
    MainWindow.cpp
    MainWindow.h
    MainWindow/Ai.cpp
    MainWindow/Feedback.cpp
    MainWindow/Project.cpp
    MainWindow/Render.cpp
    MainWindow/Render.h
    MainWindow/Ui.cpp
    MainWindow/Version.cpp
    ProjectData.cpp
    ProjectData.h
    ProjectManager/ProjectManager.cpp
    ProjectManager/ProjectManager.h
    FeedbackStore/FeedbackStore.cpp
    FeedbackStore/FeedbackStore.h
    GitService/GitService.cpp
    GitService/GitService.h
    HWFileScanner/HWFileScanner.cpp
    HWFileScanner/HWFileScanner.h
    HWpilotLLM/HWpilotLLM.cpp
    HWpilotLLM/HWpilotLLM.h
)

target_link_libraries(HWpilot PRIVATE Qt6::Widgets Qt6::Network)
