/*****************************************************************************
 *                The Virtual Light Company Copyright (c) 1999 - 2000
 *                               C Source
 *
 * This code is licensed under the GNU Library GPL. Please read license.txt
 * for the full details. A copy of the LGPL may be found at
 *
 * http://www.gnu.org/copyleft/lgpl.html
 *
 * Project:    Image Content Handlers
 * URL:        http://www.vlc.com.au/imageloader/
 *
 ****************************************************************************/

/*
 * readtiff.c
 *
 * This file contains routines to read input images in Tiff
 * format
 */

#include "decode_image.h"
#include "libtiff/libtiff/tiffio.h"

/* Error Strings */
#define ERR_TIF_NO_OPEN "Error with tiff internals"

#define ERREXIT(str) { \
              strncpy(source->pub.error_msg, str, ERROR_LEN); \
              source->pub.error_msg[ERROR_LEN-1] = '\0'; \
              source->pub.error = JNI_TRUE; \
              return; }

/* Private version of data source object */

typedef struct _tiff_source_struct * tiff_source_ptr;

typedef struct _tiff_source_struct {
  struct param pub;            /* public fields */

  int current_row;             /* current row that we should be returning */
  uint32 *raster;              /* contains all the image data in ARGB format */
} tiff_source_struct;


/*
 * Read one row of pixels.
 * This version is for reading 8-bit colormap indexes
 */
static void get_row_tiff (Parameters params)
{
   tiff_source_ptr source = (tiff_source_ptr) params;
   unsigned int i;
   uint32 tmp;
   register uint32 *inptr;
   jint *data;
   jint a, r, g, b;

   inptr = source->raster + source->pub.width * source->current_row;

   data = source->pub.buffer;

   /* put each pixel into the buffer to send back to java */
   for(i = 0; i < source->pub.width; i++) {
      /* extract components from byte array and store them */
      /* in the int array which is accessible by caller */
      /* Note that png stores bytes in RGBA format */
      tmp = *inptr++;

      a = (jint) ((tmp >> 24) & 0xff);
      r = (jint) ((tmp >>  0) & 0xff);
      g = (jint) ((tmp >>  8) & 0xff);
      b = (jint) ((tmp >> 16) & 0xff);

      /* Required to return data in ARGB format */
      data[i] = (a << 24) + (r << 16) + (g << 8) + b;
   }

   source->current_row--;
}


/*
 * Read the file header; return image size and component count.
 */
static void start_input_tiff (Parameters params)
{
   tiff_source_ptr source = (tiff_source_ptr) params;
   uint32 w, h;
   size_t npixels;
   TIFF *tif;


   /* open file with tiff fdopen */
   tif = TIFFFdOpen(fileno(source->pub.fptr), "imagefile", "rm");

   if (tif) {
      TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
      TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

      /* how many pixels in array? */
      npixels = w * h;
      source->raster = (uint32*) _TIFFmalloc(npixels * sizeof (uint32));

      if (source->raster != NULL) {
	 if (!TIFFReadRGBAImage(tif, w, h, source->raster, 0)) {
	    /* do error message here */
	    ;
	 }
      }
      else {
	 ERREXIT(ERR_OUT_OF_MEMORY);
      }

      TIFFClose(tif);

      source->current_row = h-1;

      source->pub.width = (int) w;
      source->pub.height = (int) h;
   }
   else
      ERREXIT(ERR_TIF_NO_OPEN);
}


/*
 * Finish up at the end of the file.
 */

static void finish_input_tiff (Parameters params)
{
   tiff_source_ptr source = (tiff_source_ptr) params;

   _TIFFfree(source->raster);
}

/*
 * Performs initialisation.  This function sets up necessary function pointers
 * and returns a "Parameters object" to that the calling function can access
 * the necessary internal functions of this file.
 */
Parameters tiff_init ()
{
   tiff_source_ptr source;

   /* Create module interface object */
   source = (tiff_source_ptr) malloc(sizeof(tiff_source_struct));

   if (source != NULL) {
      /* Initialise structure */
      source->pub.fptr = NULL;
      source->pub.width = -1;
      source->pub.height = -1;
      source->pub.buffer = NULL;
      source->pub.row_num = 0;
      source->pub.error = JNI_FALSE;
      source->pub.error_msg[0] = '\0';

      source->current_row = 0;
      source->raster = NULL;

      /* Fill in method ptrs */
      source->pub.start_input = start_input_tiff;
      source->pub.get_pixel_row = get_row_tiff;
      source->pub.finish_input = finish_input_tiff;
   }

   /* return the reference to initialised parameter structure */
   return (Parameters) source;
}

