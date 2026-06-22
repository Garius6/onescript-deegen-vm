# Fallback Makefile when CMake is not available.
# Mirrors vm/CMakeLists.txt for local dev on macOS without cmake installed.

CXX        ?= clang++
CXXFLAGS   ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic -Iinclude
BUILD_DIR  := build
SRC_DIR    := src
TEST_DIR   := tests

CORE_SOURCES := $(SRC_DIR)/loader.cpp $(SRC_DIR)/value.cpp $(SRC_DIR)/interp.cpp
CORE_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(CORE_SOURCES))

.PHONY: all clean test
all: $(BUILD_DIR)/osvm $(BUILD_DIR)/osdis $(BUILD_DIR)/test_loader $(BUILD_DIR)/test_interp $(BUILD_DIR)/test_fib

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/osvm: $(CORE_OBJECTS) $(SRC_DIR)/main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/osdis: $(CORE_OBJECTS) $(SRC_DIR)/disasm.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/test_loader: $(CORE_OBJECTS) $(TEST_DIR)/test_loader.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/test_interp: $(CORE_OBJECTS) $(TEST_DIR)/test_interp.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/test_fib: $(CORE_OBJECTS) $(TEST_DIR)/test_fib.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

test: all
	./$(BUILD_DIR)/test_loader
	./$(BUILD_DIR)/test_interp
	./$(BUILD_DIR)/test_fib

clean:
	rm -rf $(BUILD_DIR)
