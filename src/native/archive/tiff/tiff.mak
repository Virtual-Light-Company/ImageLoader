#
# Makefile for the TIFF library.
# Compile this under one of the myriad of Microsoft visual C compilers
#
#        nmake /f tiff.mak
#
# This will produce a library, libtiff.lib.  This can then be used to
# link in with code requiring it.
#
# Author: Dion Mendel <mendel@ccis.adisys.com.au>

# source files: TIFF library proper
SRCS = fax3sm.c tif_aux.c tif_close.c tif_codec.c \
tif_compress.c tif_dir.c tif_dirinfo.c tif_dirread.c tif_dirwrite.c \
tif_dumpmode.c tif_error.c tif_fax3.c tif_flush.c tif_getimage.c \
tif_jpeg.c tif_lzw.c tif_next.c tif_open.c tif_packbits.c \
tif_predict.c tif_print.c tif_read.c tif_strip.c tif_swab.c \
tif_thunder.c tif_tile.c tif_version.c tif_warning.c tif_custom.c \
tif_write.c tif_zip.c tif_luv.c tif_pixarlog.c

OBJS = fax3sm.obj tif_aux.obj tif_close.obj tif_codec.obj \
tif_compress.obj tif_dir.obj tif_dirinfo.obj tif_dirread.obj tif_dirwrite.obj \
tif_dumpmode.obj tif_error.obj tif_fax3.obj tif_flush.obj tif_getimage.obj \
tif_jpeg.obj tif_lzw.obj tif_next.obj tif_open.obj tif_packbits.obj \
tif_predict.obj tif_print.obj tif_read.obj tif_strip.obj tif_swab.obj \
tif_thunder.obj tif_tile.obj tif_version.obj tif_warning.obj tif_custom.obj \
tif_write.obj tif_zip.obj tif_luv.obj tif_pixarlog.obj

CC=cl
RM=del
LIBTIFF=libtiff.lib

# Optimize for speed and windows DLLs
#CFLAGS = -O2 -GD -nologo -DJPEG_SUPPORT -DZIP_SUPPORT -I../libjpeg -I ../zlib
#LDFLAGS=-L. -L../libjpeg -L../zlib -ljpeg -lzlib
CFLAGS = -O2 -GD -nologo -DBSDTYPES
LDFLAGS=-L.

all: $(LIBTIFF)

$(LIBTIFF): $(OBJS)
	lib -out:$@ $(OBJS)

clean:
	$(RM) *.obj
