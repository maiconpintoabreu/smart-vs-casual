SHELL := /bin/bash
EMSDK_PATH ?= $(abspath ../emsdk)
# Define the build directory
BUILD_DIR = build
BUILD_TYPE ?= Release  # Default to 'Debug' if BUILD_TYPE is not defined
# Default target Linux
all: configure build run
all-windows: configure-windows build-windows run-windows
all-web: configure-web build-web run-web

configure-windows:
	@mkdir -p $(BUILD_DIR)
	@cmake build . -S . -B $(BUILD_DIR) -DCMAKE_TOOLCHAIN_FILE=mingw-64.cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE}

configure:
	@mkdir -p $(BUILD_DIR)
	@cmake build . -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=${BUILD_TYPE}

configure-web:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && \
	source $(EMSDK_PATH)/emsdk_env.sh && \
	emcmake cmake .. -DPLATFORM=Web -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

# Build the project
build:
	@cd $(BUILD_DIR) && make BUILD_MODE=$(BUILD_TYPE) 

build-windows:
	@cd $(BUILD_DIR) && make PLATFORM=PLATFORM_DESKTOP OS=Windows_NT CC=x86_64-w64-mingw32-gcc BUILD_MODE=$(BUILD_TYPE) 

build-web:
	@cd $(BUILD_DIR) && \
	source $(EMSDK_PATH)/emsdk_env.sh && \
	emmake make BUILD_MODE=$(BUILD_TYPE) 
	
clean:
	mv build/_deps ./_deps_temp
	rm -rf build/*
	mkdir -p build
	mv ./_deps_temp build/_deps

run:
	./$(BUILD_DIR)/smart-vs-casual/smart-vs-casual

run-windows:
	wine ./$(BUILD_DIR)/smart-vs-casual/smart-vs-casual.exe

run-web:
	@cd ./$(BUILD_DIR)/smart-vs-casual && python3 -m http.server

.PHONY: all configure build run
.PHONY: all-windows configure-windows build-windows run-windows
.PHONY: all-web configure-web build-web run-web