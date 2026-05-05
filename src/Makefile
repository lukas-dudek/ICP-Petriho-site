# Makefile for Petri Net Interpreter
# Authors: Lukáš, Jan, Matěj
# Usage: make build, make run, make clean

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -O2
LDFLAGS  = -ldl

SRCDIR   = .
BUILDDIR = build
TARGET   = pets_backend

SOURCES = $(wildcard $(SRCDIR)/*.cpp) \
          $(wildcard $(SRCDIR)/model/*.cpp) \
          $(wildcard $(SRCDIR)/interpreter/*.cpp) \
          $(wildcard $(SRCDIR)/codegen/*.cpp) \
          $(wildcard $(SRCDIR)/ipc/*.cpp)

OBJECTS = $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SOURCES))

.PHONY: all clean run test

all: $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build OK: $(TARGET)"

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -c $< -o $@
	@echo "Compiled: $<"

clean:
	rm -rf $(BUILDDIR)
	@echo "Clean OK"

run: all
	./$(BUILDDIR)/$(TARGET)

test: all
	cd $(BUILDDIR) && ./$(TARGET)
