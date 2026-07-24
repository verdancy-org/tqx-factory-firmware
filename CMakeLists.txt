cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(ARM_NONE_EABI_GCC arm-none-eabi-gcc REQUIRED)
find_program(ARM_NONE_EABI_GXX arm-none-eabi-g++ REQUIRED)
find_program(ARM_NONE_EABI_AS arm-none-eabi-gcc REQUIRED)
set(CMAKE_C_COMPILER "${ARM_NONE_EABI_GCC}" CACHE FILEPATH "C compiler" FORCE)
set(CMAKE_CXX_COMPILER "${ARM_NONE_EABI_GXX}" CACHE FILEPATH "CXX compiler" FORCE)
set(CMAKE_ASM_COMPILER "${ARM_NONE_EABI_AS}" CACHE FILEPATH "ASM compiler" FORCE)

project(tqx_factory_firmware LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)

set(XR_LOG_MESSAGE_MAX_LEN 96 CACHE STRING "LibXR log message buffer length")

include("${CMAKE_SOURCE_DIR}/cmake/LibXR.CMake")

set(APP_SOURCES
  "${CMAKE_SOURCE_DIR}/User/main.c"
  "${CMAKE_SOURCE_DIR}/User/app_main.cpp"
  "${CMAKE_SOURCE_DIR}/Core/syscalls_stub.c"
  "${MSPM0_STARTUP_FILE}"
  "${SYSCFG_C_FILE}")

add_executable(${PROJECT_NAME}.elf ${APP_SOURCES})

target_include_directories(${PROJECT_NAME}.elf PRIVATE
  "${CMAKE_SOURCE_DIR}/Core"
  "${CMAKE_SOURCE_DIR}/User"
  ${MSPM0_COMMON_INCLUDE_DIRS})

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
  LIBXR_NOT_SUPPORT_MUTI_THREAD=1
  LIBXR_NO_EIGEN
  LIBXR_DEFAULT_SCALAR=double
  LIBXR_PRINTF_BUFFER_SIZE=128
  LIBXR_LOG_LEVEL=4
  LIBXR_LOG_OUTPUT_LEVEL=4
  $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
  ${MSPM0_COMMON_COMPILE_DEFINITIONS})

target_compile_options(${PROJECT_NAME}.elf PRIVATE
  ${MSPM0_ARCH_COMPILE_OPTIONS}
  -ffunction-sections
  -fdata-sections
  $<$<CONFIG:Debug>:-Og>
  $<$<CONFIG:Debug>:-g3>
  $<$<CONFIG:Release>:-Os>
  $<$<CONFIG:RelWithDebInfo>:-Os>
  $<$<CONFIG:RelWithDebInfo>:-g>
  $<$<CONFIG:MinSizeRel>:-Os>
  "@${SYSCFG_OUTPUT_DIR}/device.opt"
  $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
  $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
  $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>)

target_link_libraries(${PROJECT_NAME}.elf PRIVATE xr)

target_link_options(${PROJECT_NAME}.elf PRIVATE
  -mcpu=cortex-m0plus
  -mthumb
  -mfloat-abi=soft
  --specs=nano.specs
  --specs=nosys.specs
  -Wl,--gc-sections
  -Wl,--no-warn-rwx-segments
  "-Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map"
  "-T${SYSCFG_LINKER_FILE}")

find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy)
if(CMAKE_OBJCOPY)
  add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
      $<TARGET_FILE:${PROJECT_NAME}.elf>
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary
      $<TARGET_FILE:${PROJECT_NAME}.elf>
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin)
endif()
