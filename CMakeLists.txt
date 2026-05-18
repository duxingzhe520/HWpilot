cmake_minimum_required(VERSION 3.16)
project(HWpilot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 指向你本机的 Qt 安装路径
set(CMAKE_PREFIX_PATH "/Users/duxingzhe520/Qt/6.11.0/macos")

# 寻找 Qt6 组件
find_package(Qt6 COMPONENTS Core Gui Widgets Network REQUIRED)

# 源文件列表
add_executable(HWpilot
    main.cpp
    MainWindow.cpp
    MainWindow.h

    # LLM 网络请求封装
    HWpilotLLM/HWpilotLLM.cpp
    HWpilotLLM/HWpilotLLM.h
    HWpilotLLM/Prompts.h

    # 作业文件扫描器
    HWFileScanner/HWFileScanner.cpp
    HWFileScanner/HWFileScanner.h
)

# ============================================================
# 头文件搜索路径（让 #include 无需写完整子目录）
# ============================================================
target_include_directories(HWpilot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/HWpilotLLM
    ${CMAKE_CURRENT_SOURCE_DIR}/HWFileScanner
)

# ============================================================
# 链接 Qt6 库
# ============================================================
target_link_libraries(HWpilot PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)
