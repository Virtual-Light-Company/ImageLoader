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
 * readtarga.c
 *
 * This file is based on rdtarga.c from the IGL library:
 */

/****************************************************************************/
#include "decode_image.h"

/* Error Strings */

#define ERR_TGA_BADCMAP "Unsupported Targa colormap format"
#define ERR_TGA_BADPARMS "Invalid or unsupported Targa file"

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

/* Private version of data source object */

typedef struct _tga_source_struct * tga_source_ptr;

typedef struct _tga_source_struct {
   struct param pub;     /* public fields */

   U_CHAR **colormap;    /* Targa colormap (converted to my format) */

   jint **whole_image;   /* Needed if funny input row order */
   int current_row;      /* Current logical row number to read */

   /* Pointer to routine to extract next Targa pixel from input file */
   void (*read_pixel) (tga_source_ptr source);

   /* Result of read_pixel is delivered here: */
   U_CHAR tga_pixel[4];

   int pixel_size;       /* Bytes per Targa pixel (1 to 4) */

   /* State info for reading RLE-coded pixels; both counts must be init to 0 */
   int block_count;      /* # of pixels remaining in RLE block */
   int dup_pixel_count;  /* # of times to duplicate previous pixel */

   /* This saves the correct pixel-row-expansion method for preload_image */
   void (*get_pixel_row) (Parameters params);
} tga_source_struct;


/* For expanding 5-bit pixel values to 8-bit with best rounding */

static const short c5to8bits[32] = {
    0,   8,  16,  25,  33,  41,  49,  58,
   66,  74,  82,  90,  99, 107, 115, 123,
  132, 140, 148, 156, 165, 173, 181, 189,
  197, 206, 214, 222, 230, 239, 247, 255
};


/*
 * Read next byte from Targa file
 */
static int read_byte (tga_source_ptr source)
{
   register FILE *infile = source->pub.fptr;
   register int c;

   if ((c = getc(infile)) == EOF)
      ERREXIT_RET(ERR_INPUT_EOF);
   return c;
}


/*
 * Read the colormap from a Targa file
 */
static void read_colormap (tga_source_ptr source, int cmaplen, int mapentrysize)
{
   int i;

   /* Presently only handles 24-bit BGR format */
   if (mapentrysize != 24)
      ERREXIT(ERR_TGA_BADCMAP);

   for (i = 0; i < cmaplen; i++) {
      source->colormap[2][i] = (U_CHAR) read_byte(source);
      source->colormap[1][i] = (U_CHAR) read_byte(source);
      source->colormap[0][i] = (U_CHAR) read_byte(source);
   }
}


/*
 * read_pixel methods: get a single pixel from Targa file into tga_pixel[]
 * Read one Targa pixel from the input file; no RLE expansion
 */
static void read_non_rle_pixel (tga_source_ptr source)
{
   register FILE *infile = source->pub.fptr;
   register int i;

   for (i = 0; i < source->pixel_size; i++) {
      source->tga_pixel[i] = (U_CHAR) getc(infile);
   }
}


/*
 * Read one Targa pixel from the input file, expanding RLE data as needed
 */
static void read_rle_pixel (tga_source_ptr source)
{
   register FILE *infile = source->pub.fptr;
   register int i;

   /* Duplicate previously read pixel? */
   if (source->dup_pixel_count > 0) {
      source->dup_pixel_count--;
      return;
   }

   /* Time to read RLE block header? */
   if (--source->block_count < 0) {    /* decrement pixels remaining in block */
      i = read_byte(source);
      if (i & 0x80) {                     /* Start of duplicate-pixel block? */
         source->dup_pixel_count = i & 0x7F; /* number of dups after this one */
         source->block_count = 0;            /* then read new block header */
      } else {
         source->block_count = i & 0x7F;   /* number of pixels after this one */
      }
   }

   /* Read next pixel */
   for (i = 0; i < source->pixel_size; i++) {
      source->tga_pixel[i] = (U_CHAR) getc(infile);
   }
}


/*
 * Read one row of pixels.
 *
 * We provide several different versions depending on input file format.
 */


/*
 * This version is for reading 8-bit grayscale pixels
 */
static void get_8bit_gray_row (Parameters params)
{
   register int i;
   jint *data;
   jint a, r, g, b;

   tga_source_ptr source = (tga_source_ptr) params;
  
   if (source->whole_image) {
      /* whole_image exists so we assume that we are writing to the */
      /* internal buffer */
      data = source->whole_image[source->current_row];
   }
   else {
      /* we are writing to user buffer */
      data = source->pub.buffer;
   }

   /* put each pixel into the buffer to send back to java */
   for(i = 0; i < source->pub.width; i++) {
      /* extract components from byte array and store them */
      /* in the int array which is accessible by caller */
      (*source->read_pixel) (source); /* Load next pixel into tga_pixel */

      a = (jint) 255;
      r = (jint) UCH(source->tga_pixel[0]);
      g = (jint) UCH(source->tga_pixel[0]);
      b = (jint) UCH(source->tga_pixel[0]);

      /* Required to return data in ARGB format */
      data[i] = (a << 24) + (r << 16) + (g << 8) + b;
   }
}

/*
 * This version is for reading 8-bit colormap indexes
 */
static void get_8bit_row (Parameters params)
{
   register int i, t;
   jint *data;
   jint a, r, g, b;

   tga_source_ptr source = (tga_source_ptr) params;
   U_CHAR **colormap = source->colormap;
  
   if (source->whole_image) {
      /* whole_image exists so we assume that we are writing to the */
      /* internal buffer */
      data = source->whole_image[source->current_row];
   }
   else {
      /* we are writing to user buffer */
      data = source->pub.buffer;
   }

   /* put each pixel into the buffer to send back to java */
   for(i = 0; i < source->pub.width; i++) {
      /* extract components from byte array and store them */
      /* in the int array which is accessible by caller */
      (*source->read_pixel) (source); /* Load next pixel into tga_pixel */

      t = UCH(source->tga_pixel[0]);

      a = (jint) 255;
      r = (jint) colormap[0][t];
      g = (jint) colormap[1][t];
      b = (jint) colormap[2][t];

      /* Required to return data in ARGB format */
      data[i] = (a << 24) + (r << 16) + (g << 8) + b;
   }
}

/*
 * This version is for reading 16-bit pixels
 */
static void get_16bit_row (Parameters params)
{
   register int i, t;
   jint *data;
   jint a, r, g, b;
   tga_source_ptr source = (tga_source_ptr) params;
  
   if (source->whole_image) {
      /* whole_image exists so we assume that we are writing to the */
      /* internal buffer */
      data = source->whole_image[source->current_row];
   }
   else {
      /* we are writing to user buffer */
      data = source->pub.buffer;
   }

   /* put each pixel into the buffer to send back to java */
   for(i = 0; i < source->pub.width; i++) {
      /* extract components from byte array and store them */
      /* in the int array which is accessible by caller */
      (*source->read_pixel) (source); /* Load next pixel into tga_pixel */

      t = UCH(source->tga_pixel[0]);
      t += UCH(source->tga_pixel[1]) << 8;

      /* We expand 5 bit data to 8 bit sample width.
       * The format of the 16-bit (LSB first) input word is
       *     xRRRRRGGGGGBBBBB
       */
      b = (jint) c5to8bits[t & 0x1F];
      t >>= 5;
      g = (jint) c5to8bits[t & 0x1F];
      t >>= 5;
      r = (jint) c5to8bits[t & 0x1F];
      a = (jint) 255;

      /* Required to return data in ARGB format */
      data[i] = (a << 24) + (r << 16) + (g << 8) + b;
  }
}

/*
 * This version is for reading 24-bit pixels
 */
static void get_24bit_row (Parameters params)
{
   register int i, t;
   jint *data;
   jint a, r, g, b;

   tga_source_ptr source = (tga_source_ptr) params;
   U_CHAR **colormap = source->colormap;
  
   if (source->whole_image) {
      /* whole_image exists so we assume that we are writing to the */
      /* internal buffer */
      data = source->whole_image[source->current_row];
   }
   else {
      /* we are writing to user buffer */
      data = source->pub.buffer;
   }

   /* put each pixel into the buffer to send back to java */
   for(i = 0; i < source->pub.width; i++) {
      /* extract components from byte array and store them */
      /* in the int array which is accessible by caller */
      /* Note that tga is in BGR order */
      (*source->read_pixel) (source); /* Load next pixel into tga_pixel */

      t = UCH(source->tga_pixel[0]);

      a = (jint) 255;
      r = (jint) UCH(source->tga_pixel[2]);
      g = (jint) UCH(source->tga_pixel[1]);
      b = (jint) UCH(source->tga_pixel[0]);

      /* Required to return data in ARGB format */
      data[i] = (a << 24) + (r << 16) + (g << 8) + b;
   }
}

/*
 * Targa also defines a 32-bit pixel format with order B,G,R,A.
 * We presently ignore the attribute byte, so the code for reading
 * these pixels is identical to the 24-bit routine above.
 * This works because the actual pixel length is only known to read_pixel.
 */

#define get_32bit_row  get_24bit_row


/*
 * This method is for re-reading the input data in standard top-down
 * row order.  The entire image has already been read into whole_image
 * with proper conversion of pixel format, but it's in a funny row order.
 */
static void get_memory_row (Parameters params)
{
   tga_source_ptr source = (tga_source_ptr) params;

   /* Fetch that row from virtual array */
   memcpy(source->pub.buffer, source->whole_image[source->current_row],
          source->pub.width * sizeof(jint));

   /* increment row for next time */
   source->current_row++;
}


/*
 * This method loads the image into whole_image during the first call on
 * get_pixel_row.  The get_pixel_row pointer is then adjusted to call
 * get_memory_row on subsequent calls.
 */
static void preload_image (Parameters params)
{
   tga_source_ptr source = (tga_source_ptr) params;
   int row;

   /* Read the data into a virtual array in top-down row order. */
   source->current_row = source->pub.height;
   for (row = 0; row < source->pub.height; row++) {
      source->current_row--;

      /* read a row of pixels into the internal buffer */
      (*source->get_pixel_row) (params);
   }

   /* Set up to read from the virtual array in unscrambled order */
   source->pub.get_pixel_row = get_memory_row;
   source->current_row = 0;
}


/*
 * Read the file header; return image size and component count.
 */
static void start_input_tga (Parameters params)
{
   tga_source_ptr source = (tga_source_ptr) params;
   U_CHAR targaheader[18];
   int idlen, cmaptype, subtype, flags, interlace_type, components;
   unsigned int width, height, maplen;
   int is_bottom_up;

#define GET_2B(offset)((unsigned int) UCH(targaheader[offset]) + \
      (((unsigned int) UCH(targaheader[offset+1])) << 8))

   if (! ReadOK(source->pub.fptr, targaheader, 18))
      ERREXIT(ERR_INPUT_EOF);

   /* Pretend "15-bit" pixels are 16-bit --- we ignore attribute bit anyway */
   if (targaheader[16] == 15)
      targaheader[16] = 16;

   idlen = UCH(targaheader[0]);
   cmaptype = UCH(targaheader[1]);
   subtype = UCH(targaheader[2]);
   maplen = GET_2B(5);
   width = GET_2B(12);
   height = GET_2B(14);
   source->pixel_size = UCH(targaheader[16]) >> 3;
   flags = UCH(targaheader[17]);/* Image Descriptor byte */

   is_bottom_up = ((flags & 0x20) == 0);/* bit 5 set => top-down */
   interlace_type = flags >> 6;/* bits 6/7 are interlace code */

   if (cmaptype > 1 ||                    /* cmaptype must be 0 or 1 */
       source->pixel_size < 1 ||
       source->pixel_size > 4 ||
       (UCH(targaheader[16]) & 7) != 0 || /* bits/pixel must be multiple of 8 */
       interlace_type != 0)         /* currently don't allow interlaced image */
      ERREXIT(ERR_TGA_BADPARMS);

   if (subtype > 8) {
      /* It's an RLE-coded file */
      source->read_pixel = read_rle_pixel;
      source->block_count = source->dup_pixel_count = 0;
      subtype -= 8;
   } else {
      /* Non-RLE file */
      source->read_pixel = read_non_rle_pixel;
   }

   /* Now should have subtype 1, 2, or 3 */
   components = 4;           /* until proven different */

   switch (subtype) {
      case 1:/* Colormapped image */
         if (source->pixel_size == 1 && cmaptype == 1)
            source->get_pixel_row = get_8bit_row;
         else
            ERREXIT(ERR_TGA_BADPARMS);
         break;
      case 2:/* RGB image */
         switch (source->pixel_size) {
            case 2:
               source->get_pixel_row = get_16bit_row;
               break;
            case 3:
               source->get_pixel_row = get_24bit_row;
               break;
            case 4:
               source->get_pixel_row = get_32bit_row;
               break;
            default:
               ERREXIT(ERR_TGA_BADPARMS);
               break;
         }
         break;
      case 3:/* Grayscale image */
         if (source->pixel_size == 1)
            source->get_pixel_row = get_8bit_gray_row;
         else
            ERREXIT(ERR_TGA_BADPARMS);
         break;
      default:
         ERREXIT(ERR_TGA_BADPARMS);
         break;
   }

   if (is_bottom_up) {
      /* Create a virtual array to buffer the upside-down image. */
      source->whole_image = alloc2DJIntArray(height, width);

      source->pub.get_pixel_row = preload_image;
   } else {
      /* Don't need a virtual array */
      source->whole_image = NULL;
      source->pub.get_pixel_row = source->get_pixel_row;
   }

   while (idlen--)                 /* Throw away ID field */
      (void) read_byte(source);

   if (maplen > 0) {
      if (maplen > 256 || GET_2B(3) != 0)
         ERREXIT(ERR_TGA_BADCMAP);
      /* Allocate space to store the colormap */
      source->colormap = alloc2DByteArray(3, maplen);
      /* and read it from the file */
      read_colormap(source, (int) maplen, UCH(targaheader[7]));
   } else {
      if (cmaptype)/* but you promised a cmap! */
         ERREXIT(ERR_TGA_BADPARMS);
      source->colormap = NULL;
   }

   source->pub.width = width;
   source->pub.height = height;
}


/*
 * Finish up at the end of the file.
 */
static void finish_input_tga (Parameters params)
{
   tga_source_ptr source = (tga_source_ptr) params;

   /* free memory allocated to colormap */
   if (source->colormap)
      free2DByteArray(source->colormap);

   /* free memory allocated to image data */
   if (source->whole_image)
      free2DJIntArray(source->whole_image);
}


/*
 * Performs initialisation.  This function sets up necessary function pointers
 * and returns a "Parameters object" to that the calling function can access
 * the necessary internal functions of this file.
 */
Parameters targa_init ()
{
   tga_source_ptr source;

   /* Create module interface object */
   source = (tga_source_ptr) malloc(sizeof(tga_source_struct));

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
      source->colormap = NULL;
      source->whole_image = NULL;
      source->pixel_size = 0;
      source->dup_pixel_count = 0;

      /* Fill in method ptrs, except get_pixel_row which start_input sets */
      source->pub.start_input = start_input_tga;
      source->pub.finish_input = finish_input_tga;
   }

   /* return the reference to initialised parameter structure */
   return (Parameters) source;
}

