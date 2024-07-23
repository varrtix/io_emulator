# Build directory
BUILD_DIR := build

# CMake executable, default to system cmake, can be overridden by CMAKE environment variable
CMAKE ?= cmake

# CMake configuration options
JOBS := $(shell nproc 2>/dev/null || echo 2)
CMAKE_BUILD_TYPE ?= RelWithDebInfo
CMAKE_INSTALL_PREFIX ?= $(CURDIR)/dist
WITH_DRVS ?= ON
CTF_RPATH ?=

ifndef CTF_RPATH
$(error variable CTF_RPATH is not set)
endif

# Default target
all: build

# Create build directory and run CMake and Make with parallel 32
build:
	@$(CMAKE) -S . -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX) \
		-DCTF_RPATH=$(CTF_RPATH) \
		-DWITH_DRVS=$(WITH_DRVS)
	@$(CMAKE) --build $(BUILD_DIR) --parallel $(JOBS)

# Install target using CMake
install: build
	@$(CMAKE) --install $(BUILD_DIR)

# Clean build directory
clean:
	@rm -rf $(BUILD_DIR)

.PHONY: all build clean install
