# -------------------------------------------------------------------------
# These are configurable options:
# -------------------------------------------------------------------------

# Set to the location of the build directory: can be changed to be outside of
# the source tree and also allows to use different directories for different
# builds (e.g. debug and release, or using different wxWidgets versions).
#
# By default the "out" subdirectory of this directory is used.
builddir := out

#  Build tools
AR ?= ar
RANLIB ?= ranlib
CXX := c++

# Addition variables that are honoured by this makefile are:

# Standard flags for C++
CXXFLAGS ?=

# Standard preprocessor flags (common for CC and CXX)
CPPFLAGS ?=

# Standard linker flags
LDFLAGS ?=

# Compiler flags needed for expat
EXPAT_CPPFLAGS ?=

# Linker flags needed for expat
EXPAT_LDFLAGS ?= -lexpat

# Compiler flags needed for graphviz
GRAPHVIZ_CPPFLAGS ?= $(shell pkg-config libgvc --cflags)

# Linker flags needed for graphviz
GRAPHVIZ_LDFLAGS ?= $(shell pkg-config libgvc --libs)

# Set to 1 to build debug version [0,1]
DEBUG ?= 0

# Location and arguments of wx-config script
WX_CONFIG ?= wx-config

# Port of the wx library to build against [gtk1,gtk2,msw,x11,motif,osx_cocoa,osx_carbon,dfb]
WX_PORT ?= $(shell $(WX_CONFIG) --query-toolkit)

# Use DLL build of wx library to use? [0,1]
WX_SHARED ?= $(shell if test -z $(shell $(WX_CONFIG) --query-linkage); then echo 1; else echo 0; fi)

# Version of the wx library to build against.
WX_VERSION ?= $(shell $(WX_CONFIG) --query-version | sed -e 's/\([0-9]*\)\.\([0-9]*\)/\1\2/')


# -------------------------------------------------------------------------
# Lists of source files: update as they're changed/added/removed
# -------------------------------------------------------------------------

GRAPHEDITOR_SRC := \
	graphctrl.cpp \
	graphtree.cpp \
	graphprint.cpp \
	projectdesigner.cpp \
	factory.cpp \
	archive.cpp \
	tipwin.cpp

OGL_SRC := \
	basic2.cpp \
	canvas.cpp \
	divided.cpp \
	mfutils.cpp \
	basic.cpp \
	composit.cpp \
	drawn.cpp \
	ogldiag.cpp \
	bmpshape.cpp \
	constrnt.cpp \
	lines.cpp \
	oglmisc.cpp

GRAPHTEST_SRC := \
	graphtest.cpp \
	testnodes.cpp

# -------------------------------------------------------------------------
# There should be no need to modify the rest of this file
# -------------------------------------------------------------------------

### Conditionally set variables: ###

ifeq ($(WX_SHARED),0)
WX_CONFIG_SHARED_FLAG := --static=yes
endif
ifeq ($(WX_SHARED),1)
WX_CONFIG_SHARED_FLAG := --static=no
endif
ifeq ($(DEBUG),0)
OPT_AND_DEBUG_FLAGS := -DNDEBUG -O2
endif
ifeq ($(DEBUG),1)
OPT_AND_DEBUG_FLAGS := -O0 -g
endif

WX_CXXFLAGS := $(shell $(WX_CONFIG) --cxxflags $(WX_CONFIG_FLAGS))


### Helper function: ###

# This function returns the directory name only if it doesn't exist. It's meant
# to be used in the dependencies of targets creating files in this directory.
#
# Note that if we just always used the directory as dependency, all the files
# in it would be rebuilt all the time as its timestamp would be changed by
# "mkdir", making it fresher than the targets in it every time.
if_not_exists = $(filter-out $(wildcard $1),$1)


### Variables: ###

# It's important to use absolute paths everywhere so that the paths in the
# generated auto-generated files are absolute too because relative paths
# wouldn't work there if builddir is not the current directory.
top_srcdir := $(abspath ..)

CPPDEPS = -MD -MP
WX_VERSION_MAJOR := $(shell echo $(WX_VERSION) | cut -c1,1)
WX_VERSION_MINOR := $(shell echo $(WX_VERSION) | cut -c2,2)
WX_CONFIG_FLAGS := $(WX_CONFIG_UNICODE_FLAG) $(WX_CONFIG_SHARED_FLAG) \
	--toolkit=$(WX_PORT) --version=$(WX_VERSION_MAJOR).$(WX_VERSION_MINOR)

GRAPHEDITOR_CXXFLAGS := $(OPT_AND_DEBUG_FLAGS) -W -Wall \
	-I$(top_srcdir)/include -I$(top_srcdir)/ogl/include $(GRAPHVIZ_CPPFLAGS) $(EXPAT_CPPFLAGS) \
	$(WX_CXXFLAGS) $(CPPFLAGS) $(CXXFLAGS)
GRAPHEDITOR_BUILDDIR := $(builddir)/grapheditor
GRAPHEDITOR_OBJECTS := $(addprefix $(GRAPHEDITOR_BUILDDIR)/,$(GRAPHEDITOR_SRC:.cpp=.o))
GRAPHEDITOR_LIB := $(builddir)/libgrapheditor.a

OGL_CXXFLAGS := $(OPT_AND_DEBUG_FLAGS) -W -Wall -I$(top_srcdir)/include \
	-I$(top_srcdir)/ogl/include $(GRAPHVIZ_CPPFLAGS) $(EXPAT_CPPFLAGS) \
	$(WX_CXXFLAGS) $(CPPFLAGS) $(CXXFLAGS)
OGL_BUILDDIR := $(builddir)/ogl
OGL_OBJECTS :=  $(addprefix $(OGL_BUILDDIR)/,$(OGL_SRC:.cpp=.o))
OGL_LIB := $(builddir)/libogl.a

GRAPHTEST_CXXFLAGS := $(OPT_AND_DEBUG_FLAGS) -W -Wall \
	-I$(top_srcdir)/include -I$(top_srcdir)/ogl/include $(GRAPHVIZ_CPPFLAGS) $(EXPAT_CPPFLAGS) \
	$(WX_CXXFLAGS) $(CPPFLAGS) $(CXXFLAGS)
GRAPHTEST_BUILDDIR := $(builddir)/test
GRAPHTEST_OBJECTS := $(addprefix $(GRAPHTEST_BUILDDIR)/,$(GRAPHTEST_SRC:.cpp=.o))
GRAPHTEST_BIN := $(builddir)/graphtest

### Targets: ###

all: $(GRAPHTEST_BIN)

$(GRAPHEDITOR_BUILDDIR) $(OGL_BUILDDIR) $(GRAPHTEST_BUILDDIR):
	mkdir -p $@

$(GRAPHEDITOR_LIB): $(GRAPHEDITOR_OBJECTS)
	$(AR) rc $@ $(GRAPHEDITOR_OBJECTS)
	$(RANLIB) $@

$(OGL_LIB): $(OGL_OBJECTS)
	$(AR) rc $@ $(OGL_OBJECTS)
	$(RANLIB) $@

$(GRAPHTEST_BIN): $(GRAPHTEST_OBJECTS) $(GRAPHEDITOR_LIB) $(OGL_LIB)
	$(CXX) -o $@ $(GRAPHTEST_OBJECTS) $(OPT_AND_DEBUG_FLAGS) $(LDFLAGS) \
	    $(GRAPHEDITOR_LIB) $(OGL_LIB) $(GRAPHVIZ_LDFLAGS) $(EXPAT_LDFLAGS) \
	    $(shell $(WX_CONFIG) $(WX_CONFIG_FLAGS) --libs html,core,base)

$(GRAPHEDITOR_OBJECTS): $(GRAPHEDITOR_BUILDDIR)/%.o: $(top_srcdir)/src/%.cpp $(call if_not_exists,$(GRAPHEDITOR_BUILDDIR))
	$(CXX) -c -o $@ $(GRAPHEDITOR_CXXFLAGS) $(CPPDEPS) $<

$(OGL_OBJECTS): $(OGL_BUILDDIR)/%.o: $(top_srcdir)/ogl/src/%.cpp $(call if_not_exists,$(OGL_BUILDDIR))
	$(CXX) -c -o $@ $(OGL_CXXFLAGS) $(CPPDEPS) $<

$(GRAPHTEST_OBJECTS): $(GRAPHTEST_BUILDDIR)/%.o: $(top_srcdir)/samples/%.cpp $(call if_not_exists,$(GRAPHTEST_BUILDDIR))
	$(CXX) -c -o $@ $(GRAPHTEST_CXXFLAGS) $(CPPDEPS) $<

.PHONY: ogl
ogl: $(OGL_LIB)

clean:
	$(RM) $(GRAPHEDITOR_BUILDDIR)/*.[od] $(GRAPHEDITOR_LIB) \
	    $(OGL_BUILDDIR)/*.[od] $(OGL_LIB) \
	    $(GRAPHTEST_BUILDDIR)/*.[od] $(GRAPHTEST_BIN)

.PHONY: all clean

# Dependencies tracking:
-include $(GRAPHTEST_BUILDDIR)/*.d $(OGL_BUILDDIR)/*.d $(GRAPHEDITOR_BUILDDIR)/*.d
