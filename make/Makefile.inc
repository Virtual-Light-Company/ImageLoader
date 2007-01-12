#*********************************************************************
#
#                         (C) 2001 - 07 Justin Couch
#                         http://www.vlc.com.au/~justin/
#
# Lowest level common makefile for both native and Java code
# 
# Author: Justin Couch
# Version: $Revision: 1.6 $
#
#*********************************************************************

SRC_DIR=src
DOCS_DIR=docs
MAKE_DIR=make

JAVA_DIR=$(SRC_DIR)/java
NATIVE_DIR=$(SRC_DIR)/native
INCLUDE_DIR=$(NATIVE_DIR)/include

DELETE    = rm -f
MAKEDIR   = mkdir -p
RMDIR     = rm -fr
PRINT     = @echo
COPY      = cp 
COPY_PATH = cp --parents
CHMOD     = chmod
CD        = cd

#JAVAC_FLAGS=-deprecation
#JAVADOC_FLAGS=

MAKEFLAGS=-s
