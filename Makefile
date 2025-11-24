.PHONY: all clean init deinit
vpath %.h $(INCLUDE_DIR)

TARGET_EXEC := vfs
SRC_DIR := ./src
INCLUDE_DIR := ./include
BIN_DIR := ./bin
BUILD_DIR := ./build


INC_FILES := $(shell find $(INCLUDE_DIR) -type f -name "*.h")
INC_FLAGS := $(addprefix -I,$(INCLUDE_DIR))
SRCS := $(shell find $(SRC_DIR) -type f -name "*.cpp" -exec basename {} \;)
OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

CXX = g++
CXXFLAGS = --std=c++17 $(INC_FLAGS)
SHELL := $(shell command -v zsh || command -v bash)
# SHELL = /bin/bash


all: $(BIN_DIR)/$(TARGET_EXEC)
	@echo "Run 'source $(BIN_DIR)/init.sh' to launch filesystem."

run: all
	$(BIN_DIR)/$(TARGET_EXEC)

# ifeq ($(origin VFS_FILESYSTEM_PATH), undefined)
# 	@. $(BIN_DIR)/init.sh
# endif
# ifeq ($(origin VFS_FILESYSTEM), undefined)
# 	@. $(BIN_DIR)/init.sh
# endif

# init:
# 	@. $(BIN_DIR)/init.sh
# deinit:
# 	@. $(BIN_DIR)/deinit.sh

$(BIN_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp $(INCLUDE_DIR)/diskLibrary.h $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/diskLibrary.o: $(SRC_DIR)/diskLibrary.cpp $(INC_FILES) $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir ./build

clean:
	@if [ -n "$(VFS_FILESYSTEM_PATH)" ] && [ -n "$(VFS_FILESYSTEM)" ] && \
	     [ -d "$(VFS_FILESYSTEM_PATH)/.$(VFS_FILESYSTEM)" ]; then \
	    rm -r "$(VFS_FILESYSTEM_PATH)/.$(VFS_FILESYSTEM)"; \
	fi
	@if [ -d "$(BUILD_DIR)" ]; then \
	    rm -r "$(BUILD_DIR)"; \
	fi
	@if [ -f "$(BIN_DIR)/$(TARGET_EXEC)" ]; then \
		rm "$(BIN_DIR)/$(TARGET_EXEC)"; \
	fi
	@echo "Run 'source $(BIN_DIR)/deinit.sh' to restore shell."
# 	@. $(BIN_DIR)/deinit.sh