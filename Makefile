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

TARGET               := $(BUILD_DIR)/bminor$(EXE_EXT)
TEST_LEXER_BIN       := $(BUILD_DIR)/lexer_tests$(EXE_EXT)
TEST_PARSER_BIN      := $(BUILD_DIR)/parser_tests$(EXE_EXT)
TEST_HIGHLIGHTER_BIN := $(BUILD_DIR)/highlighter_tests$(EXE_EXT)

LIB_SRCS := $(wildcard src/*/*.cpp)
LIB_OBJS := $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(LIB_SRCS))
MAIN_OBJ := $(OBJ_DIR)/main.o

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(MAIN_OBJ) $(LIB_OBJS)
	@$(call MKDIR_P,$(dir $@))
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: src/%.cpp
	@$(call MKDIR_P,$(dir $@))
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_LEXER_BIN): tests/lexer_tests.cpp $(LIB_OBJS)
	@$(call MKDIR_P,$(dir $@))
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_PARSER_BIN): tests/parser_tests.cpp $(LIB_OBJS)
	@$(call MKDIR_P,$(dir $@))
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_HIGHLIGHTER_BIN): tests/highlighter_tests.cpp $(LIB_OBJS)
	@$(call MKDIR_P,$(dir $@))
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

test: $(TEST_LEXER_BIN) $(TEST_PARSER_BIN) $(TEST_HIGHLIGHTER_BIN)
	$(TEST_LEXER_BIN)
	$(TEST_PARSER_BIN)
	$(TEST_HIGHLIGHTER_BIN)

run: $(TARGET)
	$(TARGET)

clean:
	@$(call RM_RF,$(BUILD_DIR))
