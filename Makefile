#
# Top Level makefile for the Imageloader project from The VLC.
#
# To use this, make sure that you have the PROJECT_ROOT environment variable
# set 
#
# This makefile is designed to build the entire library from scratch. It is
# not desigend as a hacking system. It is recommended that you use the normal
# javac/CLASSPATH setup for that. 
#
# The following commands are offered:
# 
# - jar:      Make the java JAR file 
# - javadoc:  Generate the javadoc information
# - lib:      Make the native DLL to load images
# - all:      Build everything (including docs)
# - nuke:     Blow everything away
#

ifndef PROJECT_ROOT
export PROJECT_ROOT=/projects/imageloader
endif

include $(PROJECT_ROOT)/make/Makefile.inc

VERSION=1.1

all: jar lib javadoc

jar:
	cd $(JAVA_DIR) && make buildall

javadoc:
	cd $(JAVA_DIR) && make javadoc

lib:
	cd $(JAVA_DIR) && make jni
	cd $(NATIVE_DIR) && make all
    
