# Minimal toolchain file for arm-none-eabi
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Avoid trying to link executables when cross-compiling (prevents host link errors)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Specify compilers (adjust path if your toolchain is elsewhere)
set(CMAKE_C_COMPILER "D:/download/stm32_vscode_envirorment/10 2021.10/bin/arm-none-eabi-gcc.exe")
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")
set(CMAKE_OBJCOPY "D:/download/stm32_vscode_envirorment/10 2021.10/bin/arm-none-eabi-objcopy.exe")
set(CMAKE_SIZE "D:/download/stm32_vscode_envirorment/10 2021.10/bin/arm-none-eabi-size.exe")
set(CMAKE_AR "D:/download/stm32_vscode_envirorment/10 2021.10/bin/arm-none-eabi-ar.exe")

# Optionally set sysroot or flags here
# set(CMAKE_C_FLAGS "-mcpu=cortex-m3 -mthumb -Os -g")

# Prevent CMake from searching for host system libraries
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
