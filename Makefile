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

all: jar libs javadoc

class:
	cd $(JAVA_DIR) && make buildall

jar:
	cd $(JAVA_DIR) && make buildall
	cd $(JAVA_DIR) && make jar

javadoc:
	cd $(JAVA_DIR) && make javadoc

jni:
	cd $(JAVA_DIR) && make jni

libs:
	cd $(JAVA_DIR) && make nativeall
	cd $(NATIVE_DIR) && make buildall
    
clean:
	cd $(JAVA_DIR) && make clean
	cd $(NATIVE_DIR) && make clean
