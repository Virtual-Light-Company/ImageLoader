#*********************************************************************
#
#  (C) 2002 Justin Couch
#  http://www.vlc.com.au/~justin/
# 
# Package makefile for the vlc.net.content.image directory
#
# Author: Justin Couch
# Version: $Revision: 1.2 $
#
#*********************************************************************

PACKAGE = vlc.net.content.image

# The source files to compile. These are declared in order that they must be
# compiled in
SOURCE = ImageBuffer.java \
         ImageDecoder.java \
		 BufferFiller.java \
		 ImageBuilder.java \
         bmp.java \
         gif.java \
         jpeg.java \
         png.java \
         targa.java \
         tiff.java \
         x_portable_graymap.java \
         x_portable_pixmap.java

# Source files to create the JNI header information from
JNI_SOURCE=ImageDecoder.java

# The list of other files we need to copy from this directory to the classes
# directory when we are making JAR files.
EXTRA = 

# If the package requires extra classes outside the JDK, the JAR files that 
# are required are listed here. These should be found in ${PROJECT_ROOT}/lib
JARS_3RDPARTY =

include ${PROJECT_ROOT}/make/Makefile.jav
