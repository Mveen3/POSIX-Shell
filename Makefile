# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic -g -Iinclude

# Source files
SOURCES = src/core/main.cpp \
          src/core/Shell.cpp \
          src/core/Signals.cpp \
          src/core/Utils.cpp \
          src/parser/Parser.cpp \
          src/execution/Pipeline.cpp \
          src/execution/Redirection.cpp \
          src/execution/SystemCommands.cpp \
          src/builtins/Builtins.cpp \
          src/builtins/History.cpp \
          src/builtins/Pinfo.cpp \
          src/builtins/Search.cpp \
          src/terminal/InputHandler.cpp \
          src/terminal/Autocomplete.cpp

# Build directory and object files
BUILD_DIR = build
OBJECTS = $(patsubst src/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))
DEPS = $(OBJECTS:.o=.d)

# Target executable name
TARGET = shell

# Default target
all: $(TARGET)

# Link executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)

# Compile source files to object files in build directory
$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Include generated header dependency files
-include $(DEPS)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Run automated verification test suite
test: $(TARGET)
	python3 tests/test_suite.py


# Build and run with AddressSanitizer & UndefinedBehaviorSanitizer
sanitize: CXXFLAGS += -fsanitize=address,undefined
sanitize: clean all test

# Install (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Phony targets
.PHONY: all clean install test sanitize