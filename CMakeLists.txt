cmake_minimum_required(VERSION 3.20)

project(ums)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickTimeline ShaderTools)

add_executable(ums
    src/main.cpp
    src/core/umsd.cpp
    src/core/ui.cpp
    src/ui/thermal_presenter.cpp
    src/subsystems/subsystem.cpp
    src/subsystems/thermal.cpp
    src/wrappers/thermal_wrapper.cpp
    src/wrappers/MLX90640_I2C_Driver.c
    third_party/mlx90640/MLX90640_API.c
)

target_include_directories(ums PRIVATE
    src/core
    src/ui
    src/subsystems
    src/wrappers
    third_party/mlx90640
)

target_compile_options(ums PRIVATE
    -Wall
    -Wextra
)

add_subdirectory(Qt/Ums/Ums)
add_subdirectory(Qt/Ums/UmsContent)
add_subdirectory(Qt/Ums/Dependencies/Components)

target_link_libraries(ums PRIVATE
    pthread
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Quick
    Qt6::Qml
    Ums
    UmsContent
    Umsplugin
    UmsContentplugin
    QtQuickDesignerComponents
)
