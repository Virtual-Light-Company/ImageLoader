#*********************************************************************
#
#                         (C) 2001-02 Justin Couch
#                         http://www.vlc.com.au/~justin/
#
# Lowest level common makefile for both native and Java code
# 
# Author: Justin Couch
# Version: $Revision: 1.2 $
#
#*********************************************************************

ifndef PROJECT_ROOT
  export PROJECT_ROOT=/usr/local/src/projects/imageloader
endif

ifndef JAVA_HOME
  JAVA_HOME=/usr/local/java/jdk1.3
endif

SRC_DIR=$(PROJECT_ROOT)/src
DOCS_DIR=$(PROJECT_ROOT)/docs
LIB_DIR=$(PROJECT_ROOT)/lib
MAKE_DIR=$(PROJECT_ROOT)/make

JAVA_DIR=$(SRC_DIR)/java
NATIVE_DIR=$(SRC_DIR)/native
INCLUDE_DIR=$(NATIVE_DIR)/include

DELETE    = rm -f
MAKEDIR   = mkdir -p
RMDIR     = rm -fr
PRINT     = @echo
COPY      = cp 
COPY_PATH = cp -P
CHMOD     = chmod
CD        = cd

MAKEFLAGS=-s
