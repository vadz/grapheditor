Overview
========

This is a simple graph editor library for wxWidgets.


Building
========

Prerequisites
-------------

1. wxWidgets 3.x
1. Graphviz headers and libraries: currently tested with Graphviz 2.26 only.


Using Microsoft Visual Studio
-----------------------------

The solution file uses MSVS 2017 format, but previous MSVS 201x versions should
probably work as well -- they haven't been tested however.

1. Edit `build/graphviz.props` file and change GraphvizDir directory value to
   point to Graphviz installation directory (note that this value should be
   backslash-terminated).
1. Open `build/grapheditor.sln` and build it. Currently only 32-bit build is
   supported.
1. Run `graphtest` sample after setting the `WX_GRAPHTEST_DATA_DIR` environment
   variable to point to `samples/resources` directory to allow it finding its
   image files.
