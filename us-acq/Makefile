# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Iinclude

# App name
APPNAME = us_acq

# Directories
SRCDIR = src
INCDIR = include
BUILDDIR = build

# Sources and objects
SRC = $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SRC))

# Default target
all: $(APPNAME).exe

# Link step
$(APPNAME).exe: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile step (make sure build dir exists)
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create build directory if missing
$(BUILDDIR):
	mkdir $(BUILDDIR)

# Clean
clean:
	del /Q $(BUILDDIR)\*.o $(APPNAME).exe 2>nul || true
