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

#include "decode_image.h"

/* Portions of this code are based on the PBMPLUS library, which is:
**
** Copyright (C) 1988 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

/* Error strings */
#define ERR_PPM_NONNUMERIC "Nonnumeric data in PPM file"
#define ERR_PPM_NOT "Not a PPM file"

#define ERREXIT(str) { \
              strncpy(source->pub.error_msg, str, ERROR_LEN); \
              source->pub.error_msg[ERROR_LEN-1] = '\0'; \
              source->pub.error = JNI_TRUE; \
              return; }

#define ERREXIT_RET(str) { \
              strncpy(source->pub.error_msg, str, ERROR_LEN); \
              source->pub.error_msg[ERROR_LEN-1] = '\0'; \
              source->pub.error = JNI_TRUE; \
              return 1; }

/*
 * On most systems, reading individual bytes with getc() is drastically less
 * efficient than buffering a row at a time with fread().  On PCs, we must
 * allocate the buffer in near data space, because we are assuming small-data
 * memory model, wherein fread() can't reach far memory.  If you need to
 * process very wide images on a PC, you might have to compile in large-memory
 * model, or else replace fread() with a getc() loop --- which will be much
 * slower.
 */


/* Private version of data source object */

typedef struct {
   struct param pub;            /* public fields */

   U_CHAR *iobuffer;            /* non-FAR pointer to I/O buffer */
   size_t buffer_width;         /* width of I/O buffer */
   U_CHAR *rescale;             /* => maxval-remapping array, or NULL */
} ppm_source_struct;

typedef ppm_source_struct * ppm_source_ptr;

/*
 * Read next char, skipping over any comments
 * A comment/newline sequence is returned as a newline
 */
static int pbm_getc (ppm_source_ptr source)
{
   register FILE *infile = source->pub.fptr;
   register int ch;

   ch = getc(infile);
   if (ch == '#') {
      do {
         ch = getc(infile);
      } while (ch != '\n' && ch != EOF);
   }
   return ch;
}


/*
 * Read an unsigned decimal integer from the PPM file
 * Swallows one trailing character after the integer
 * Note that on a 16-bit-int machine, only values up to 64k can be read.
 * This should not be a problem in practice.
 */
static unsigned int read_pbm_integer (ppm_source_ptr source)
{
   register FILE *infile = source->pub.fptr;
   register int ch;
   register unsigned int val;

   /* Skip any leading whitespace */
   do {
      ch = pbm_getc(source);
      if (ch == EOF)
	 ERREXIT_RET(ERR_INPUT_EOF);
   } while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');

   if (ch < '0' || ch > '9')
      ERREXIT_RET(ERR_PPM_NONNUMERIC);

   val = ch - '0';
   while ((ch = pbm_getc(source)) >= '0' && ch <= '9') {
      val *= 10;
      val += ch - '0';
   }
   return val;
}


/*
 * Read one row of pixels.
 *
 * We provide several different versions depending on input file format.
 */

/*
 * This version is for reading text-format PGM files with any maxval
 */
static void get_text_gray_row (Parameters params)
{
   ppm_source_ptr source = (ppm_source_ptr) params;
   register U_CHAR *rescale = source->rescale;
   register int i;
   jint *data;
   jint a, r, g, b;
   U_CHAR tmp;

   data = source->pub.buffer;

   /* put each pixel into the buffer to send back to java */
   for(i = 0; i < source->pub.width; i++) {
      /* extract components from byte array and store them */
      /* in the int array which is accessible by caller */
      tmp = rescale[read_pbm_integer(source)];

      a = (jint) 255;
      r = (jint) tmp;
      g = (jint) tmp;
      b = (jint) tmp;

      /* Required to return data in ARGB format */
      data[i] = (a << 24) + (r << 16) + (g << 8) + b;
   }
}


/*
 * This version is for reading text-format PPM files with any maxval
 */
static void get_text_rgb_row (Parameters params)
{
   ppm_source_ptr source = (ppm_source_ptr) params;
   register U_CHAR *rescale = source->rescale;
   register int i;
   jint *data;
   jint a, r, g, b;

   data = source->pub.buffer;

   /* put each pixel into the buffer to send back to java */
   for(i = 0; i < source->pub.width; i++) {
      /* extract components from byte array and store them */
      /* in the int array which is accessible by caller */

      a = (jint) a;
      r = (jint) rescale[read_pbm_integer(source)];
      g = (jint) rescale[read_pbm_integer(source)];
      b = (jint) rescale[read_pbm_integer(source)];

      /* Required to return data in ARGB format */
      data[i] = (a << 24) + (r << 16) + (g << 8) + b;
   }
}


/*
 * This version is for reading raw-byte-format PGM files with any maxval
 */
static void get_scaled_gray_row (Parameters params)
{
   ppm_source_ptr source = (ppm_source_ptr) params;
   register U_CHAR * bufferptr;
   register U_CHAR *rescale = source->rescale;
   register int i;
   jint *data;
   jint a, r, g, b;
   U_CHAR tmp;

   if (! ReadOK(source->pub.fptr, source->iobuffer, source->buffer_width))
      ERREXIT(ERR_INPUT_EOF);

   data = source->pub.buffer;

   bufferptr = source->iobuffer;
   for(i = 0; i < source->pub.width; i++) {
      /* extract components from byte array and store them */
      /* in the int array which is accessible by caller */
      tmp = rescale[UCH(*bufferptr++)];

      a = (jint) 255;
      r = (jint) tmp;
      g = (jint) tmp;
      b = (jint) tmp;

      /* Required to return data in ARGB format */
      data[i] = (a << 24) + (r << 16) + (g << 8) + b;
   }
}


/*
 * This version is for reading raw-byte-format PPM files with any maxval
 */
static void get_scaled_rgb_row (Parameters params)
{
   ppm_source_ptr source = (ppm_source_ptr) params;
   register U_CHAR * bufferptr;
   register U_CHAR *rescale = source->rescale;
   register int i;
   jint *data;
   jint a, r, g, b;

   if (! ReadOK(source->pub.fptr, source->iobuffer, source->buffer_width))
      ERREXIT(ERR_INPUT_EOF);

   data = source->pub.buffer;

   bufferptr = source->iobuffer;
   for(i = 0; i < source->pub.width; i++) {
      /* extract components from byte array and store them */
      /* in the int array which is accessible by caller */
      a = (jint) 255;
      r = (jint) rescale[UCH(*bufferptr++)];
      g = (jint) rescale[UCH(*bufferptr++)];
      b = (jint) rescale[UCH(*bufferptr++)];

      /* Required to return data in ARGB format */
      data[i] = (a << 24) + (r << 16) + (g << 8) + b;
   }
}


/*
 * This version is for reading raw-word-format PGM files with any maxval
 */
static void get_word_gray_row (Parameters params)
{
   ppm_source_ptr source = (ppm_source_ptr) params;
   register U_CHAR * bufferptr;
   register U_CHAR *rescale = source->rescale;
   register int i;
   jint *data;
   jint a, r, g, b;
   U_CHAR tmp;

   if (! ReadOK(source->pub.fptr, source->iobuffer, source->buffer_width))
      ERREXIT(ERR_INPUT_EOF);

   data = source->pub.buffer;

   bufferptr = source->iobuffer;
   for(i = 0; i < source->pub.width; i++) {
      register int temp;
      /* extract components from byte array and store them */
      /* in the int array which is accessible by caller */

      temp  = UCH(*bufferptr++);
      temp |= UCH(*bufferptr++) << 8;
      tmp = rescale[temp];
      a = (jint) 255;
      r = (jint) tmp;
      g = (jint) tmp;
      b = (jint) tmp;

      /* Required to return data in ARGB format */
      data[i] = (a << 24) + (r << 16) + (g << 8) + b;
   }
}


/*
 * This version is for reading raw-word-format PPM files with any maxval
 */
static void get_word_rgb_row (Parameters params)
{
   ppm_source_ptr source = (ppm_source_ptr) params;
   register U_CHAR * bufferptr;
   register U_CHAR *rescale = source->rescale;
   register int i;
   jint *data;
   jint a, r, g, b;

   if (! ReadOK(source->pub.fptr, source->iobuffer, source->buffer_width))
      ERREXIT(ERR_INPUT_EOF);

   data = source->pub.buffer;

   bufferptr = source->iobuffer;
   for(i = 0; i < source->pub.width; i++) {
      register int temp;
      /* extract components from byte array and store them */
      /* in the int array which is accessible by caller */
      a = (jint) 255;
      temp  = UCH(*bufferptr++);
      temp |= UCH(*bufferptr++) << 8;
      r = (jint) rescale[temp];
      temp  = UCH(*bufferptr++);
      temp |= UCH(*bufferptr++) << 8;
      g = (jint) rescale[temp];
      temp  = UCH(*bufferptr++);
      temp |= UCH(*bufferptr++) << 8;
      b = (jint) rescale[temp];

      /* Required to return data in ARGB format */
      data[i] = (a << 24) + (r << 16) + (g << 8) + b;
   }
}


/*
 * Read the file header; return image size and component count.
 */
static void start_input_ppm (Parameters params)
{
   ppm_source_ptr source = (ppm_source_ptr) params;
   int c;
   int w, h, maxval;
   int need_iobuffer, need_rescale;
   int input_components;

   if (getc(source->pub.fptr) != 'P')
      ERREXIT(ERR_PPM_NOT);

   c = getc(source->pub.fptr); /* save format discriminator for a sec */

   /* fetch the remaining header info */
   w = read_pbm_integer(source);
   h = read_pbm_integer(source);
   maxval = read_pbm_integer(source);

   if (w <= 0 || h <= 0 || maxval <= 0) /* error check */
      ERREXIT(ERR_PPM_NOT);

   source->pub.width = (int) w;
   source->pub.height = (int) h;

   /* initialize flags to most common settings */
   need_iobuffer = JNI_TRUE;                /* do we need an I/O buffer? */
   need_rescale = JNI_TRUE;                 /* do we need a rescale array? */

   switch (c) {
      case '2':                        /* it's a text-format PGM file */
         input_components = 1;
	 source->pub.get_pixel_row = get_text_gray_row;
	 need_iobuffer = JNI_FALSE;
	 break;

      case '3':                        /* it's a text-format PPM file */
         input_components = 3;
	 source->pub.get_pixel_row = get_text_rgb_row;
	 need_iobuffer = JNI_FALSE;
	 break;

      case '5':                        /* it's a raw-format PGM file */
         input_components = 1;
	 if (maxval > 255) {
	    source->pub.get_pixel_row = get_word_gray_row;
	 } else {
	    source->pub.get_pixel_row = get_scaled_gray_row;
	 }
	 break;

      case '6':                        /* it's a raw-format PPM file */
         input_components = 3;
	 if (maxval > 255) {
	    source->pub.get_pixel_row = get_word_rgb_row;
	 } else {
	    source->pub.get_pixel_row = get_scaled_rgb_row;
	 }
	 break;

      default:
	 ERREXIT(ERR_PPM_NOT);
	 break;
   }

   /* Allocate space for I/O buffer: 1 or 3 bytes or words/pixel. */
   if (need_iobuffer) {
      source->buffer_width = (size_t) w * input_components *
	 ((maxval<=255) ? sizeof(U_CHAR) : (2*sizeof(U_CHAR)));

      source->iobuffer = (U_CHAR *)
                           malloc(source->buffer_width * sizeof(U_CHAR));
      if (!source->iobuffer)
         ERREXIT(ERR_OUT_OF_MEMORY);
   }

   /* Compute the rescaling array if required. */
   if (need_rescale) {
      int val, half_maxval;

      /* On 16-bit-int machines we have to be careful of maxval = 65535 */
      source->rescale = (U_CHAR *) malloc (maxval+1 * sizeof (U_CHAR));

      /* check memory allocation */
      if (!source->rescale) {
         if (source->iobuffer)
            free(source->iobuffer);

         ERREXIT(ERR_OUT_OF_MEMORY);
      }

      half_maxval = maxval / 2;
      for (val = 0; val <= maxval; val++) {
	 /* The multiplication here must be done in 32 bits to avoid overflow */
	 source->rescale[val] = (U_CHAR) ((val*255 + half_maxval)/maxval);
      }
   }
}


/*
 * Finish up at the end of the file.
 */
static void finish_input_ppm (Parameters params)
{
   ppm_source_ptr source = (ppm_source_ptr) params;

   /* free rescale buffer */
   if (source->rescale)
      free(source->rescale);

   /* free io buffer */
   if (source->iobuffer)
      free(source->iobuffer);
}


/*
 * Performs initialisation.  This function sets up necessary function pointers
 * and returns a "Parameters object" to that the calling function can access
 * the necessary internal functions of this file.
 */
Parameters ppm_init ()
{
   ppm_source_ptr source;

   /* Create module interface object */
   source = (ppm_source_ptr) malloc(sizeof(ppm_source_struct));

   if (source != NULL) {
      /* Initialise structure */
      source->pub.fptr = NULL;
      source->pub.width = -1;
      source->pub.height = -1;
      source->pub.buffer = NULL;
      source->pub.row_num = 0;
      source->pub.error = JNI_FALSE;
      source->pub.error_msg[0] = '\0';

      source->iobuffer = NULL;
      source->buffer_width = 0;
      source->rescale = NULL;

      /* Fill in method ptrs, except get_pixel_row which start_input sets */
      source->pub.start_input = start_input_ppm;
      source->pub.finish_input = finish_input_ppm;
   }

   /* return the reference to initialised parameter structure */
   return (Parameters) source;
}

