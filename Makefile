# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic -g

# Source files
SOURCES = main.cpp Shell.cpp Parser.cpp Builtins.cpp SystemCommands.cpp \
          Redirection.cpp Pipeline.cpp Utils.cpp History.cpp Pinfo.cpp \
          Search.cpp Autocomplete.cpp InputHandler.cpp Signals.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Dependency files
DEPS = $(SOURCES:.cpp=.d)

# Executable name
TARGET = shell

# Default target
all: $(TARGET)

# Create executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)

# Include dependencies
-include $(DEPS)

# Compile source files to object files and generate dependencies
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Clean up
clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)

# Install (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Phony targets
.PHONY: all clean install