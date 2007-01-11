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
export PROJECT_ROOT=$(PWD)
endif

include $(PROJECT_ROOT)/make/Makefile.inc

# Default instruction is to print out the help list
help:
	$(PRINT) 
	$(PRINT) "                   The VLC Imageloader Project"
	$(PRINT) 
	$(PRINT) "More information on this project can be found at"
	$(PRINT) "http://www.vlc.com.au/imageloader"
	$(PRINT) 
	$(PRINT) "The following options are offered and will build the entire codebase:"
	$(PRINT) 
	$(PRINT) "class:       Compile just the classes. Don't make JAR files."
	$(PRINT) "bin:         Build parsers and classes"
	$(PRINT) "jar:         Make the java JAR file"
	$(PRINT) "javadoc:     Generate the javadoc information"
	$(PRINT) "libs:         Generate the native libraries"
	$(PRINT) "jni:         Build just the JNI interfaces"
	$(PRINT) "all:         Build everything (including docs)"
	$(PRINT) "clean:       Blow all the library classes away"
	$(PRINT) 


all: jar lib javadoc

class:
	make -f $(JAVA_DIR)/Makefile buildall

jar:
	make -f $(JAVA_DIR)/Makefile jar

javadoc:
	make -f $(JAVA_DIR)/Makefile javadoc

jni:
	make -f $(JAVA_DIR)/Makefile jni

libs:
	make -f $(JAVA_DIR)/Makefile nativeall
	make -f $(NATIVE_DIR)/Makefile buildall

clean:
	make -f $(JAVA_DIR)/Makefile clean
	make -f $(NATIVE_DIR)/Makefile clean
