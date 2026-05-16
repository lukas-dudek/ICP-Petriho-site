# Makefile for Petri Net Interpreter & GUI
# Authors: Lukáš, Jan, Matěj

CXX       = g++
MOC       = moc
QT_CXXFLAGS = $(shell pkg-config --cflags Qt5Widgets)
QT_LIBS     = $(shell pkg-config --libs Qt5Widgets)
CXXFLAGS  = -std=c++17 -Wall -Wextra -pthread -O2 -fPIC $(QT_CXXFLAGS) -I.
LDFLAGS   = -ldl $(QT_LIBS)

SRCDIR    = .
BUILDDIR  = build
TARGET    = pets_app

# Backend sources (NO main_backend.cpp – that has its own main())
SOURCES_BACKEND = $(wildcard $(SRCDIR)/model/*.cpp) \
                  $(wildcard $(SRCDIR)/interpreter/*.cpp) \
                  $(wildcard $(SRCDIR)/codegen/*.cpp) \
                  $(wildcard $(SRCDIR)/ipc/*.cpp)

# GUI sources
SOURCES_GUI     = $(wildcard $(SRCDIR)/gui/*.cpp) \
                  $(wildcard $(SRCDIR)/gui/canvas/*.cpp) \
                  $(wildcard $(SRCDIR)/gui/widgets/*.cpp)

SOURCES = $(SOURCES_BACKEND) $(SOURCES_GUI)
OBJECTS = $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SOURCES))

# Headers that need MOC (contain Q_OBJECT)
MOC_HEADERS = $(SRCDIR)/gui/main_window.h \
              $(SRCDIR)/gui/canvas/petrinet_scene.h \
              $(SRCDIR)/gui/canvas/petrinet_view.h \
              $(SRCDIR)/gui/widgets/injection_panel.h \
              $(SRCDIR)/gui/widgets/log_panel.h \
              $(SRCDIR)/gui/widgets/state_overview_panel.h

MOC_SOURCES = $(patsubst $(SRCDIR)/%.h, $(BUILDDIR)/moc_%.cpp, $(MOC_HEADERS))
MOC_OBJECTS = $(patsubst %.cpp, %.o, $(MOC_SOURCES))

.PHONY: all clean run test

all: $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET): $(OBJECTS) $(MOC_OBJECTS)
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build OK: $(TARGET)"

# Compile normal .cpp -> .o
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generate MOC files from headers with Q_OBJECT
$(BUILDDIR)/moc_%.cpp: $(SRCDIR)/%.h
	@mkdir -p $(dir $@)
	$(MOC) $(QT_CXXFLAGS) $< -o $@

# Compile MOC .cpp -> .o
$(BUILDDIR)/moc_%.o: $(BUILDDIR)/moc_%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)
	@echo "Clean OK"

run: all
	./$(BUILDDIR)/$(TARGET)

test: all
	cd $(BUILDDIR) && ./$(TARGET)