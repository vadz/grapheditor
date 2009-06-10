##############################################################################
# Name:         docs/Makefile
# Purpose:      makefile for building doxygen docs
# Author:       Vadim Zeitlin
# Created:      2006-03-10
# RCS-ID:       $Id$
# Copyright:    (c) 2006 TT-Solutions SARL
##############################################################################

# set this to the location of your HTML Help compiler
HHC := hhc.exe

DOXYGEN := doxygen.exe


all: chm

chm: graph.chm

html: html/index.hhp

graph.chm: html/index.hhp
	@# hhc always returns error exit code (thanks Microsoft) so ignore it
	-$(HHC) $<

html/index.hhp: graph.doxy ../include/*.h ../samples/graphtest.cpp
	$(DOXYGEN) graph.doxy

clean:
	$(RM) -r graph.chm html

.PHONY: all chm clean
