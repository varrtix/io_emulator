cmake_minimum_required(VERSION 3.16)

project(io_test LANGUAGES CXX)

include(GNUInstallDirs)
include(FetchContent)
include(${CMAKE_SOURCE_DIR}/external/drv_emu/cmake/ctf.cmake)
include(${CMAKE_SOURCE_DIR}/external/drv_emu/cmake/emu.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable AddressSanitizer for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling AddressSanitizer for Debug build")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

# Set the policy CMP0135 to NEW
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Wall)
        add_compile_definitions(__GNU_C__)
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_GENERATOR_PLATFORM x64)
    add_compile_options(/W4)
    add_compile_definitions(WIN32 CTF_DRIVER_EXPORTS)
endif()


# Fetch the GNU Readline library
FetchContent_Declare(
    readline
    URL https://ftp.gnu.org/gnu/readline/readline-8.2.tar.gz
    URL_HASH SHA256=3feb7171f16a84ee82ca18a36d7b9be109a52c04f492a053331d7d1095007c35
)
FetchContent_MakeAvailable(readline)

file(GLOB SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cc)
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/external/include
  ${readline_SOURCE_DIR}
  ${ctf_path}/include
  ${ctf_path}/include/xml
  ${DRV_DEPS_INCLUDE}
)
target_link_directories(${PROJECT_NAME} PRIVATE ${ctf_path}/bin/Debug)
target_link_libraries(${PROJECT_NAME} PRIVATE readline iosvc)
