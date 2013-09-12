# -*- Mode: makefile -*-
# Makefile for GAlib
# Copyright (c) 1996-2005 Matthew Wall, all rights reserved
#
# If you need to customize the build of galib, you should first modify the
# variables in the makevars file.

GALIB_VERSION=2.4.7
GALIB_VER=247
TMPDIR=/var/tmp
RELDIR=$(TMPDIR)/galib$(GALIB_VER)

all: lib

lib:
	cd ga; $(MAKE)

clean:
	cd ga; $(MAKE) clean
