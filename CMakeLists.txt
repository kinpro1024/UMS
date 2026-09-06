cmake_minimum_required(VERSION 3.20)

project(ums)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ums
    src/main.cpp
    src/subsystems/subsystem.cpp
    src/subsystems/thermal.cpp
    src/wrappers/thermal_wrapper.cpp
    src/core/umsd.cpp
    third_party/mlx90640/MLX90640_API.c
    src/wrappers/MLX90640_I2C_Driver.c
)

target_include_directories(ums PRIVATE
    src/core
    src/subsystems
    src/wrappers
    third_party/mlx90640
)

target_compile_options(ums PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(ums PRIVATE
    pthread
)
