.PHONY: all build cmake clean format

BUILD_DIR := build
BUILD_TYPE ?= Debug
SHELL := C:\Windows\System32\WindowsPowerShell\v1.0
CMAKE_PROJECT_NAME := usb-magnetometer.embedded

all: build

${BUILD_DIR}/Makefile:
	cmake \
		-B ${BUILD_DIR} \
		-D CMAKE_BUILD_TYPE=${BUILD_TYPE} \
		-D CMAKE_TOOLCHAIN_FILE=gcc-arm-none-eabi.cmake \
		-D CMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-G "Unix Makefiles"

cmake: ${BUILD_DIR}/Makefile

build: cmake
	$(MAKE) -C ${BUILD_DIR} --no-print-directory

clean:
	rmdir /s /q .\$(BUILD_DIR)\


#######################################
# flash using JLink
#######################################
DEVICE = STM32L010C6
$(BUILD_DIR)/jflash: $(BUILD_DIR)/$(CMAKE_PROJECT_NAME).bin
	type nul > $@
	@echo SelectInterface 1 >> $@
	@echo Speed 4000 >> $@
	@echo LoadFile $< 0x08000000 >> $@
	@echo Reset >> $@
	@echo Go >> $@
	@echo Exit >> $@

$(BUILD_DIR)/$(CMAKE_PROJECT_NAME).bin: build

jflash: $(BUILD_DIR)/jflash
	JLink.exe -device $(DEVICE) -CommandFile $<
