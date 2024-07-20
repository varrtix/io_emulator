cmake_minimum_required(VERSION 3.16)

project(io_test LANGUAGES CXX)

include(GNUInstallDirs)
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

set(CTF_LIBRARIES 
    iosvc
    ctf_util
    ctf_log
)

# Set the policy CMP0135 to NEW
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Enable AddressSanitizer for Debug builds
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Enabling AddressSanitizer for Debug build")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Wall)
        add_compile_definitions(__GNU_C__)
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    set(CMAKE_GENERATOR_PLATFORM x64)
    add_compile_definitions(WIN32 _CRT_SECURE_NO_WARNINGS)

    if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        add_compile_options(/W4)
        set(tmp_list ${CTF_LIBRARIES})
        set(CTF_LIBRARIES)
        foreach(ctf_libname IN LISTS tmp_list)
            find_library(${ctf_libname}_LIB NAMES ${ctf_libname} PATHS ${CTF_LIBDIR})
            list(APPEND CTF_LIBRARIES ${${ctf_libname}_LIB})
            message(STATUS "Appending Library: ${${ctf_libname}_LIB}")
        endforeach()
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-Wall)
    endif()
endif()

if (DEFINED READLINE_PATH)
    find_path(READLINE_INCLUDE_DIRS readline/readline.h PATHS ${READLINE_PATH}/include)
    find_library(READLINE_LIBRARIES NAMES readline PATHS ${READLINE_PATH}/lib)
    if (NOT READLINE_INCLUDE_DIRS OR NOT READLINE_LIBRARIES)
        message(FATAL_ERROR "Readline library not found.")
    else()
        message(STATUS "Found readline include: ${READLINE_INCLUDE_DIRS}")
        message(STATUS "Found readline library: ${READLINE_LIBRARIES}")
    endif()
else()
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(READLINE REQUIRED readline)
        if(NOT READLINE_FOUND)
            message(FATAL_ERROR "Readline library not found.")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        message(FATAL_ERROR "'READLINE_PATH' not set.")
    endif()
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
)
target_link_directories(${PROJECT_NAME} PRIVATE ${CTF_LIBDIR})
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CTF_LIBRARIES}
    ${READLINE_LIBRARIES}
)
