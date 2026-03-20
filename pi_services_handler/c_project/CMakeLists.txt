cmake_minimum_required(VERSION 3.10)
project(pi_services_handler_c C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Optionally enable optimizations
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic -O3 -march=native)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(pi_services_handler
    src/main.c
    src/motion_control.c
    src/socket_comm.c
    src/serial_comm.c
    src/data_parser.c
    src/joystick.c
    src/qmc5883p.c
    src/pid_controller.c
)

# 链接数学库和线程库
find_package(Threads REQUIRED)
target_link_libraries(pi_services_handler PRIVATE m Threads::Threads)

# 如果需要串口和网络库，glibc自带，无需额外
