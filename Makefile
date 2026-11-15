CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS  ?=

BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj

ifeq ($(OS),Windows_NT)
    EXE_EXT := .exe
    MKDIR_P = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
    RM_RF   = if exist "$(subst /,\,$(1))" rmdir /S /Q "$(subst /,\,$(1))"
else
    EXE_EXT :=
    MKDIR_P = mkdir -p "$(1)"
    RM_RF   = rm -rf "$(1)"
endif

TARGET := $(BUILD_DIR)/bminor$(EXE_EXT)

SRCS := $(wildcard src/*.cpp src/*/*.cpp)
OBJS := $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	@$(call MKDIR_P,$(dir $@))
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: src/%.cpp
	@$(call MKDIR_P,$(dir $@))
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	$(TARGET)

clean:
	@$(call RM_RF,$(BUILD_DIR))
