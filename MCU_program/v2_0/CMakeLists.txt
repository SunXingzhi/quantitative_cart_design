cmake_minimum_required(VERSION 3.14)

project(Quantitative_Cart_Firmware LANGUAGES C ASM)

# Default toolchain: arm-none-eabi (can be overridden via -DCMAKE_TOOLCHAIN_FILE or CMAKE_C_COMPILER)
if(NOT CMAKE_TOOLCHAIN_FILE)
	# You can pass -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake to use the provided toolchain file
endif()

if(NOT CMAKE_C_COMPILER)
	set(CMAKE_C_COMPILER "arm-none-eabi-gcc" CACHE FILEPATH "C compiler")
endif()
if(NOT CMAKE_ASM_COMPILER)
	set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}" CACHE FILEPATH "ASM compiler")
endif()

set(CMAKE_OBJCOPY "arm-none-eabi-objcopy")
set(CMAKE_SIZE "arm-none-eabi-size")
set(CMAKE_AR "arm-none-eabi-ar")

# CPU / MCU flags for STM32F1 (Cortex-M3)
set(MCU_FLAGS "-mcpu=cortex-m3 -mthumb")
set(COMMON_FLAGS "${MCU_FLAGS} -fdata-sections -ffunction-sections")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -Os -g")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

# Define target device macro expected by vendor headers
add_compile_definitions(STM32F10X_MD)
# Enable inclusion of the Standard Peripheral drivers (so stm32f10x.h pulls in
# the peripheral headers like stm32f10x_gpio.h and defines assert_param)
add_compile_definitions(USE_STDPERIPH_DRIVER)

# Include directories (adjust if your tree differs)
include_directories(
	${CMAKE_SOURCE_DIR}/Standard_periphal_library/include
	${CMAKE_SOURCE_DIR}/core_periphal
	${CMAKE_SOURCE_DIR}/Hardware_driver
	${CMAKE_SOURCE_DIR}/Software_package
	${CMAKE_SOURCE_DIR}/User
	${CMAKE_SOURCE_DIR}/System
	${CMAKE_SOURCE_DIR}/Start
	${CMAKE_SOURCE_DIR}/RTE/Device
)

# Collect sources (recursive). If you prefer an explicit list, replace this with explicit file lists.
file(GLOB_RECURSE SRC_C
	"${CMAKE_SOURCE_DIR}/*.c"
)
file(GLOB_RECURSE SRC_S
	"${CMAKE_SOURCE_DIR}/*.s"
	"${CMAKE_SOURCE_DIR}/*.S"
)

set(SRC_FILES ${SRC_C} ${SRC_S})

if(NOT SRC_FILES)
	message(WARNING "No source files found. Please adjust the GLOB or add sources manually.")
endif()

add_executable(${PROJECT_NAME}.elf ${SRC_FILES})

# Some CMSIS core implementations may generate inline-asm patterns that
# trigger assembler register overlap with certain compiler/optimization
# settings. Force a different optimization level for the core source to
# avoid the 'strexb/strexh registers may not be the same' assembler error.
set_source_files_properties(${CMAKE_SOURCE_DIR}/Start/core_cm3.c PROPERTIES COMPILE_FLAGS "-O2")

# Linker script: set -DLINKER_SCRIPT=path/to/your_linker.ld or place one under linker_scripts/
if(NOT DEFINED LINKER_SCRIPT)
	set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker_scripts/stm32f103.ld)
endif()

target_link_options(${PROJECT_NAME}.elf PRIVATE "-T${LINKER_SCRIPT}")

# Post build: produce .bin and .hex and show size
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
	COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
	COMMAND ${CMAKE_OBJCOPY} -O ihex	 $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
	COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
	COMMENT "Generating binary/hex and printing size"
)

# Convenient alias
add_custom_target(bin ALL DEPENDS ${PROJECT_NAME}.elf)
