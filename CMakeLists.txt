cmake_minimum_required(VERSION 3.16)

project(io_test LANGUAGES CXX)

include(GNUInstallDirs)
# include(FetchContent)
include(${CMAKE_SOURCE_DIR}/external/drv_emu/cmake/emu.cmake)
include(${DRV_EMU_DIR}/cmake/ctf.cmake)

set_stdcxx()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define the macro for enabling or disabling CTFConsole
option(ENABLE_CTF_CLI "Disable CTFConsole mode" ON)
# Add a definition based on the option
if(ENABLE_CTF_CLI)
    add_definitions(-DCTF_CLI)
    message(STATUS "Enabling definition flag: CTF_CLI")
endif()

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
# FetchContent_Declare(
#     readline
#     URL https://ftp.gnu.org/gnu/readline/readline-8.2.tar.gz
#     URL_HASH SHA256=3feb7171f16a84ee82ca18a36d7b9be109a52c04f492a053331d7d1095007c35
# )

# FetchContent_Declare(
#     bdb
#     URL https://fossies.org/linux/misc/db-18.1.40.tar.gz
#     URL_HASH SHA256=0cecb2ef0c67b166de93732769abdeba0555086d51de1090df325e18ee8da9c8
# )

# FetchContent_MakeAvailable(readline bdb)

find_package(PkgConfig REQUIRED)
pkg_check_modules(READLINE REQUIRED readline)
if(NOT READLINE_FOUND)
    message(FATAL_ERROR "Readline library not found.")
endif()

find_path(BDB_INCLUDE_DIR NAMES db_cxx.h)
find_library(BDB_LIBRARY NAMES db_cxx-5.3)
if(NOT BDB_INCLUDE_DIR OR NOT BDB_LIBRARY)
    message(FATAL_ERROR "Could not find Berkeley DB library or include directory")
endif()

file(GLOB SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cc)
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/include
    ${ctf_path}/include
    ${ctf_path}/include/xml
    ${DRV_DEPS_INCLUDE}
    ${READLINE_INCLUDE_DIRS}
    ${BDB_INCLUDE_DIR}
)
target_link_directories(${PROJECT_NAME} PRIVATE ${ctf_path}/bin/Debug)
target_link_libraries(${PROJECT_NAME} PRIVATE
    iosvc
    ctf_util
    ctf_log
    ${READLINE_LIBRARIES}
    ${BDB_LIBRARY}
)
