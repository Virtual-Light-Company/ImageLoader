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
 * readbmp.c
 *
 * This file contains routines to read input images in Microsoft "BMP"
 * format (MS Windows 3.x, OS/2 1.x, and OS/2 2.x flavors).
 */

/****************************************************************************/
#include "decode_image.h"

/* Error Strings */

#define ERR_BMP_BADCMAP "Unsupported BMP colormap format"
#define ERR_BMP_BADDEPTH "Only 8- and 24-bit BMP files are supported"
#define ERR_BMP_BADHEADER "Invalid BMP file: bad header length"
#define ERR_BMP_BADPLANES "Invalid BMP file: biPlanes not equal to 1"
#define ERR_BMP_NOT "Is not a BMP file"
#define ERR_BMP_UNSUPPORTED "16 colour compressed BMP file not supported"

#define ERREXIT(str) { \
              strncpy(source->pub.error_msg, str, ERROR_LEN); \
              source->pub.error_msg[ERROR_LEN-1] = '\0'; \
              source->pub.error = JNI_TRUE; \
              return; }

#define ERREXIT_RET(str) { \
              strncpy(source->pub.error_msg, str, ERROR_LEN); \
              source->pub.error_msg[ERROR_LEN-1] = '\0'; \
              source->pub.error = JNI_TRUE; \
              return(0); }

/* Private version of data source object */

typedef struct _bmp_source_struct * bmp_source_ptr;

typedef struct _bmp_source_struct {
   struct param pub;             /* public fields */

   U_CHAR **colormap;            /* BMP colormap (converted to my format) */

   U_CHAR **whole_image;         /* Needed to reverse row order */
   int source_row;               /* Current source row number */
   int row_width;                /* Physical width of scanlines in file */

   int bits_per_pixel;           /* remembers 1-, 2-, 4-, 8- or 24-bit format */
   int compression;              /* remembers 0, 1, 2 compression */
   int image_size;               /* bytes of image data in the file */
} bmp_source_struct;


/*
 * Read next byte from BMP file
 */
static int read_byte (bmp_source_ptr source)
{
   register FILE *infile = source->pub.fptr;
   register int c;

   if ((c = getc(infile)) == EOF)
      ERREXIT_RET(ERR_INPUT_EOF);
   return c;
}

/*
 * Read next short from BMP file
 */
static int read_short (bmp_source_ptr source)
{
   register FILE *infile = source->pub.fptr;
   register unsigned int lower;
   register int upper;

   if ((lower = getc(infile)) == EOF)
      ERREXIT_RET(ERR_INPUT_EOF);
   if ((upper = getc(infile)) == EOF)
      ERREXIT_RET(ERR_INPUT_EOF);

   return (upper << 8) + lower;
}


/*
 * Read the colormap from a BMP file
 */
static void read_colormap (bmp_source_ptr source, int cmaplen, int mapentrysize)
{
   int i;

   switch (mapentrysize) {
      case 3:
         /* BGR format (occurs in OS/2 files) */
         for (i = 0; i < cmaplen; i++) {
            source->colormap[2][i] = (U_CHAR) read_byte(source);
            source->colormap[1][i] = (U_CHAR) read_byte(source);
            source->colormap[0][i] = (U_CHAR) read_byte(source);
         }
         break;
      case 4:
         /* BGR0 format (occurs in MS Windows files) */
         for (i = 0; i < cmaplen; i++) {
            source->colormap[2][i] = (U_CHAR) read_byte(source);
            source->colormap[1][i] = (U_CHAR) read_byte(source);
            source->colormap[0][i] = (U_CHAR) read_byte(source);
            (void) read_byte(source);
         }
         break;
      default:
         ERREXIT(ERR_BMP_BADCMAP);
         break;
   }
}

/*
 * This method loads the image into whole_image.  
 * The data in the file is RLE encoded, so this routine decodes it.
 * The algorithm for decoding RLE was achieved by a combination of examining
 * other (buggy) RLE code, reading the (sometimes incorrect) tech info from
 * Micro$oft's site, and reverse engineering the image format from sample
 * images.
 */
static void extract_rle_data (Parameters params)
{
   bmp_source_ptr source = (bmp_source_ptr) params;
   register U_CHAR *out_ptr;
   register FILE *infile = source->pub.fptr;

   int row, col;
   int imageSize, i, j;
   int xoff, yoff;
   int currByte, byte1, byte2;
   int pixelSize;
 
   imageSize = source->image_size;

   if (source->compression == 1)
      pixelSize = 8;
   else
      pixelSize = 4;

   col = 0;
   row = 0;

   /* load first row */
   out_ptr = source->whole_image[row];

   /* You already know how many bytes are in the image, so only go through */
   /* that many. */

   i=0;
   while (i < imageSize) {
      /* RLE encoding is defined by two bytes */
      if ((byte1 = getc(infile)) == EOF)
         ERREXIT(ERR_INPUT_EOF);
      if ((byte2 = getc(infile)) == EOF)
         ERREXIT(ERR_INPUT_EOF);

      i += 2;

      /* If byte 1 == 0, this is an escape code */
      if (byte1 == 0) {

         /* If escaped, byte 2 == 0 means you are at end of line */
         if (byte2 == 0) {
            col = 0;
            row++;

            out_ptr = source->whole_image[row];
         }
         /* If escaped, byte 2 == 1 means end of bitmap */
         else if (byte2 == 1) {
            return;
         }
         /* if escaped, byte 2 == 2 adjusts the current x and y by */
         /* an offset stored in the next two bytes */
         else if (byte2 == 2) {
            xoff = read_byte(source);
            i++;
            yoff = read_byte(source);
            i++;
            col += xoff;
            row += yoff;

            out_ptr = &(source->whole_image[row][col]);
         }
         /* If escaped, any other value for byte 2 is the number of */
         /* samples that you should read as pixel values (these pixels */
         /* are not run-length encoded) */
         else {

            for (j=0; j < byte2; j++) {

               // Read in the next byte
               if ((currByte = getc(infile)) == EOF)
                  ERREXIT(ERR_INPUT_EOF);
               i++;

               /* necessary to prevent buffer overrun */
               /* incase image file is corrupt */
               if (col >= source->pub.width) {
                  continue;
               }

               if (pixelSize == 4) {
                  *out_ptr++ = (U_CHAR) ((currByte >> 4) & 0xf);
                  col++;
                  j++;

                  if (j < byte2) {
                     *out_ptr++ = (U_CHAR) (currByte & 0xf);
                     col++;
                  }
               }
               else {
                  *out_ptr++ = (U_CHAR) currByte;
                  col++;
               }
            }
            /* The pixels must be word-aligned, so if you read */
            /* an uneven number of bytes, read and ignore a byte */
            /* to get aligned again. */

            /* determine if we need to read another byte */
            if (pixelSize == 4)
               byte2 >>= 1;

            if ( (byte2 & 1) == 1) {
               if (getc(infile) == EOF)
                  ERREXIT(ERR_INPUT_EOF);
               i++;
            }
         }

      }
      /* If the first byte was not 0, it is the number of samples that */
      /* are encoded by byte 2 */
      else {
         for (j=0; j < byte1; j++) {

            /* necessary to prevent buffer overrun */
            /* incase image file is corrupt */
            if (col >= source->pub.width) {
               continue;
            }

            if (pixelSize == 4) {
               /* If j is odd, use the upper 4 bits */
               if ((j & 1) == 0) {
                  *out_ptr++ = (U_CHAR) ((byte2 >> 4) & 0xf);
               } else {
                  *out_ptr++ = (U_CHAR) (byte2 & 0xf);
               }
            } else {
               *out_ptr++ = (U_CHAR) byte2; 
            }
            col++;
         }
      }
   }
}

/*
 * Read one row of pixels.
 * The image has been read into the whole_image array, but is otherwise
 * unprocessed.  We must read it out in top-to-bottom row order, and if
 * it is an 8-bit image, we must expand colormapped pixels to 24bit format.
 */
static void get_nbit_row (Parameters params)
/* This version is for reading 8-bit colormap indexes */
{
   bmp_source_ptr source = (bmp_source_ptr) params;
   register U_CHAR **colormap = source->colormap;
   register int t;
   register U_CHAR *inptr;
   register jint *data;
   register int col;
   jint a, r, g, b;

   /* Fetch next row from virtual array */
   source->source_row--;

   /* Expand the colormap indexes to real data */
   inptr = source->whole_image[source->source_row];
   data = source->pub.buffer;

   for (col = 0; col < source->pub.width; col++) {
      /* extract components from byte array and store them *
      /* in the int array which is accessible by caller */
      a = (jint)(255);

      t = (int)(inptr[col] & 0xff);

      r = (jint)((int)(colormap[0][t]) & 0xff);
      g = (jint)((int)(colormap[1][t]) & 0xff);
      b = (jint)((int)(colormap[2][t]) & 0xff);

/*
For 16 bits per pixel
      a = (unsigned int)(1);
      r = ((unsigned int)((colormap[0][t]) >> 3) & 0x1f);
      g = ((unsigned int)((colormap[1][t]) >> 3) & 0x1f);
      b = ((unsigned int)((colormap[2][t]) >> 3) & 0x1f);
*/

      /* Required to return data in ARGB format */
      data[col] = (a << 24) + (r << 16) + (g << 8) + b;
/*
      data[col] = (jshort)((a << 15) + (r << 10) + (g << 5) + b);
*/
   }
}


/*
 * This version is for reading 24-bit pixels
 */
static void get_24bit_row (Parameters params)
{
   bmp_source_ptr source = (bmp_source_ptr) params;
   register U_CHAR *inptr;
   register jint *data;
   register int col;
   jint a, r, g, b;

   /* Fetch next row from virtual array */
   source->source_row--;

   /* Transfer data.  Note source values are in BGR order
    * (even though Microsoft's own documents say the opposite).
    */
   inptr = source->whole_image[source->source_row];
   data = source->pub.buffer;

   for (col = 0; col < source->pub.width; col++) {

      /* extract components from byte array and store them *
      /* in the short array which is accessible by caller */
      a = (jint)(255);

      b = (jint)((int)(*inptr++) & 0xff);
      g = (jint)((int)(*inptr++) & 0xff);
      r = (jint)((int)(*inptr++) & 0xff);

/*
      a = (unsigned int)(1);
      b = ((unsigned int)(*inptr++ >> 3) & 0x1f);
      g = ((unsigned int)(*inptr++ >> 3) & 0x1f);
      r = ((unsigned int)(*inptr++ >> 3) & 0x1f);
*/

      /* Required to return data in ARGB format */
      data[col] = (a << 24) + (r << 16) + (g << 8) + b;
/*
      data[col] = (jshort)((a << 15) + (r << 10) + (g << 5) + b);
*/
   }
}


/*
 * This method loads the image into whole_image during the first call on
 * get_pixel_rows.  The get_pixel_rows pointer is then adjusted to call
 * get_8bit_row or get_24bit_row on subsequent calls.
 */
static void preload_image (Parameters params)
{
   bmp_source_ptr source = (bmp_source_ptr) params;
   register FILE *infile = source->pub.fptr;
   register int c;
   register U_CHAR *out_ptr;
   int row, col;
   int pixels_per_byte, bit_mask, bit_shift, bits_per_pixel;
   int num_cols, i;
 
   bits_per_pixel = source->bits_per_pixel;   /* local copy for faster access */

   /* get data into whole_image buffer */
   switch(source->compression) {
   case 0:
      num_cols = source->row_width;

      /* this is for the case where there are multiple pixels stored per byte */
      if (bits_per_pixel < 8) {
         bit_mask = (1 << bits_per_pixel) - 1;

         pixels_per_byte = 8 / bits_per_pixel;
         num_cols = source->pub.width / pixels_per_byte;

         /* make sure we round up */
         if ((source->pub.width % pixels_per_byte) != 0)
            num_cols++;

         while ((num_cols & 3) != 0) num_cols++;
      }

      /* Read the data into a virtual array in input-file row order. */
      for(row = 0; row < source->pub.height; row++) {

         out_ptr = source->whole_image[row];

         for(col = num_cols; col > 0; col--) {
            /* inline copy of read_byte() for speed */
            if ((c = getc(infile)) == EOF)
               ERREXIT(ERR_INPUT_EOF);

             /* extract, so that one byte has only one pixel */
             if (bits_per_pixel < 8) {
                bit_shift = 8 - bits_per_pixel;
                for(i=0;i<pixels_per_byte;i++) {
                   *out_ptr++ = (U_CHAR) ( (c >> bit_shift) & bit_mask );
                   bit_shift -= bits_per_pixel;
                }
             }
             else
                *out_ptr++ = (U_CHAR) c;
         }
      }
      break;
   case 1:
   case 2:
      extract_rle_data(params);
      break;
   default:
      break;
   }

   /* Set up to read from the virtual array in top-to-bottom order */
   switch (bits_per_pixel) {
   case 1:
   case 2:
   case 4:
   case 8:
      source->pub.get_pixel_row = get_nbit_row;
      break;
   case 24:
      source->pub.get_pixel_row = get_24bit_row;
      break;
   default:
      ERREXIT(ERR_BMP_BADDEPTH);
   }
   source->source_row = source->pub.height;

   /* And read the first row */
   (*source->pub.get_pixel_row) (params);
}


/*
 * Read the file header; return image size and component count.
 */
static void start_input_bmp (Parameters params)
{
   bmp_source_ptr source = (bmp_source_ptr) params;
   U_CHAR bmpfileheader[14];
   U_CHAR bmpinfoheader[64];
#define GET_2B(array,offset)  ((unsigned int) UCH(array[offset]) + \
                              (((unsigned int) UCH(array[offset+1])) << 8))
#define GET_4B(array,offset)  ((int) UCH(array[offset]) + \
                              (((int) UCH(array[offset+1])) << 8) + \
                              (((int) UCH(array[offset+2])) << 16) + \
                              (((int) UCH(array[offset+3])) << 24))
   int bfOffBits;
   int headerSize;
   int biWidth = 0;               /* initialize to avoid compiler warning */
   int biHeight = 0;
   unsigned int biPlanes;
   int biCompression;
   int biSizeImage;
   int biXPelsPerMeter,biYPelsPerMeter;
   int biClrUsed = 0;
   int mapentrysize = 0;          /* 0 indicates no colormap */
   int bPad;
   int row_width;

   /* Read and verify the bitmap file header */
   if (! ReadOK(source->pub.fptr, bmpfileheader, 14))
      ERREXIT(ERR_INPUT_EOF);
   if (GET_2B(bmpfileheader,0) != 0x4D42) /* 'BM' */
      ERREXIT(ERR_BMP_NOT);
   bfOffBits = (int) GET_4B(bmpfileheader,10);
   /* We ignore the remaining fileheader fields */

   /* The infoheader might be 12 bytes (OS/2 1.x), 40 bytes (Windows),
    * or 64 bytes (OS/2 2.x).  Check the first 4 bytes to find out which.
    */
   if (! ReadOK(source->pub.fptr, bmpinfoheader, 4))
      ERREXIT(ERR_INPUT_EOF);
   headerSize = (int) GET_4B(bmpinfoheader,0);
   if (headerSize < 12 || headerSize > 64)
      ERREXIT(ERR_BMP_BADHEADER);
   if (! ReadOK(source->pub.fptr, bmpinfoheader+4, headerSize-4))
      ERREXIT(ERR_INPUT_EOF);

   switch ((int) headerSize) {
   case 12:
      /* Decode OS/2 1.x header (Microsoft calls this a BITMAPCOREHEADER) */
      biWidth = (int) GET_2B(bmpinfoheader,4);
      biHeight = (int) GET_2B(bmpinfoheader,6);
      biPlanes = GET_2B(bmpinfoheader,8);
      source->bits_per_pixel = (int) GET_2B(bmpinfoheader,10);

      switch (source->bits_per_pixel) {
      case 1:/* colormapped image */
      case 2:/* colormapped image */
      case 4:/* colormapped image */
      case 8:/* colormapped image */
         mapentrysize = 3;            /* OS/2 uses RGBTRIPLE colormap */
         break;
      case 24:     /* RGB image */
         break;
      default:
         ERREXIT(ERR_BMP_BADDEPTH);
         break;
      }
      if (biPlanes != 1) {
         ERREXIT(ERR_BMP_BADPLANES);
      }
      break;
   case 40:
   case 64:
      /* Decode Windows 3.x header (Microsoft calls this a BITMAPINFOHEADER) */
      /* or OS/2 2.x header, which has additional fields that we ignore */
      biWidth = GET_4B(bmpinfoheader,4);
      biHeight = GET_4B(bmpinfoheader,8);
      biPlanes = GET_2B(bmpinfoheader,12);
      source->bits_per_pixel = (int) GET_2B(bmpinfoheader,14);
      biCompression = GET_4B(bmpinfoheader,16);
      biSizeImage = GET_4B(bmpinfoheader, 20);
      biXPelsPerMeter = GET_4B(bmpinfoheader,24);
      biYPelsPerMeter = GET_4B(bmpinfoheader,28);
      biClrUsed = GET_4B(bmpinfoheader,32);

      /* biClrImportant field is ignored */

      switch (source->bits_per_pixel) {
      case 1:/* colormapped image */
      case 2:/* colormapped image */
      case 4:/* colormapped image */
      case 8:/* colormapped image */
         mapentrysize = 4;     /* Windows uses RGBQUAD colormap */
         break;
      case 24:                 /* RGB image */
         break;
      default:
         ERREXIT(ERR_BMP_BADDEPTH);
         break;
      }
      if (biPlanes != 1)
         ERREXIT(ERR_BMP_BADPLANES);

      break;
   default:
      ERREXIT(ERR_BMP_BADHEADER);
      break;
   }

   /* Compute distance to bitmap data --- will adjust for colormap below */
   bPad = bfOffBits - (headerSize + 14);

   /* Read the colormap, if any */
   if (mapentrysize > 0) {
      if (biClrUsed <= 0)
         biClrUsed = 1 << source->bits_per_pixel;   /* assume it's 256 */
      else if (biClrUsed > 256)
         ERREXIT(ERR_BMP_BADCMAP);
      /* Allocate space to store the colormap */
      if ( !(source->colormap = alloc2DByteArray(3, (int) biClrUsed)))
         ERREXIT(ERR_OUT_OF_MEMORY);

      /* and read it from the file */
      read_colormap(source, (int) biClrUsed, mapentrysize);
      /* account for size of colormap */
      bPad -= biClrUsed * mapentrysize;
   }

   /* Skip any remaining pad bytes */
   if (bPad < 0){             /* incorrect bfOffBits value? */
      ERREXIT(ERR_BMP_BADHEADER);
   }
   while (--bPad >= 0) {
      (void) read_byte(source);
   }

   /* Compute row width in file, including padding to 4-byte boundary */
   if (source->bits_per_pixel == 24)
      row_width = (int) (biWidth * 3);
   else if (source->bits_per_pixel == 8)
      row_width = (int) biWidth;
   else
      row_width = (int) biWidth + 3 * 8;    /* extra for safety */

   while ((row_width & 3) != 0) row_width++;
   source->row_width = row_width;

   /* Allocate space for inversion array, prepare for preload pass */
   if ( !(source->whole_image = alloc2DByteArray(biHeight, row_width)))
      ERREXIT(ERR_OUT_OF_MEMORY);

   source->pub.get_pixel_row = preload_image;

   source->compression = biCompression;
   source->image_size = biSizeImage;

   /* set image width and height */
   source->pub.width = (int) biWidth;
   source->pub.height = (int) biHeight;
}


/*
 * Finish up at the end of the file.
 */
static void finish_input_bmp (Parameters params)
{
   bmp_source_ptr source = (bmp_source_ptr) params;

   /* free allocated memory */
   free2DByteArray(source->colormap);
   free2DByteArray(source->whole_image);

   /* Must remember to free the parameter structure as well */
   free(source);
}


/*
 * Performs initialisation.  This function sets up necessary function pointers
 * and returns a "Parameters object" to that the calling function can access
 * the necessary internal functions of this file.
 */
Parameters bmp_init ()
{
   bmp_source_ptr source;

   /* Create module interface object */
   source = (bmp_source_ptr) malloc(sizeof(bmp_source_struct));

   if (source != NULL) {
      /* Initialise structure */
      source->pub.fptr = NULL;
      source->pub.width = -1;
      source->pub.height = -1;
      source->pub.buffer = NULL;
      source->pub.row_num = 0;
      source->pub.error = JNI_FALSE;
      source->pub.error_msg[0] = '\0';

      source->row_width = 0;
      source->source_row = 0;
      source->colormap = NULL;
      source->whole_image = NULL;
      source->image_size = 0;

      /* Fill in method ptrs, except get_pixel_row which start_input sets */
      source->pub.start_input = start_input_bmp;
      source->pub.finish_input = finish_input_bmp;
   }

   /* return the reference to initialised parameter structure */
   return (Parameters) source;
}

