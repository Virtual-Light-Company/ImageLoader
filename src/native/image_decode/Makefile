#*********************************************************************
#
#  (C) 2002 Justin Couch
#  http://www.vlc.com.au/~justin/
# 
# Makefile for the image_decode native library
#
# Author: Justin Couch
# Version: $Revision: 1.3 $
#
#*********************************************************************

# The output library that we want to create
LIBRARY=image_decode

# source files:
C_SOURCE = common.c \
	decode_image.c \
	readppm.c \
	readtiff.c \
	readtarga.c \
	readbmp.c \
	readjpeg.c \
	readpng.c \
    image_scale_filter.c \
    area_avg_scale_filter.c \

# Other libraries that the compile process is dependent on
LIBRARY_3RDPARTY=z png jpeg tiff

include ${PROJECT_ROOT}/make/Makefile.native
