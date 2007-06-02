#*********************************************************************
#
#  (C) ?
# 
# Package makefile for the vlc.image directory
#
# Author: Rex Melton
# Version: $Revision: 1.3 $
#
#*********************************************************************

PACKAGE = vlc.image

# The source files to compile. These are declared in order that they must be
# compiled in
SOURCE = \
  ByteBufferImage.java \
  ImageScaleFilterDriver.java \
  ImageScaleFilter.java \

# Source files to create the JNI header information from
JNI_SOURCE=ImageScaleFilterDriver.java


# The list of other files we need to copy from this directory to the classes
# directory when we are making JAR files.
EXTRA = 

JARS_3RDPARTY=

include ${PROJECT_ROOT}/make/Makefile.jav
