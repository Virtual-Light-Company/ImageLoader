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

/*****************************************************************************/
#ifndef _DECODE_IMAGE_H
#define _DECODE_IMAGE_H

typedef struct param* Parameters;

/* Structure to hold available image types */
typedef struct {
   Parameters (*init_func)();     /* Initialisation function for image type */
   char *type_string;             /* Image type as a string */
} KnownImageType;

/*****************************************************************************/
/*      Modify this section when adding support for new image formats        */

/* External reference to image initialisation functions */
extern Parameters png_init();
extern Parameters bmp_init();
extern Parameters jpeg_init();
extern Parameters targa_init();
extern Parameters ppm_init();
extern Parameters tiff_init();

/* Number of image formats that we support */
#define NUM_KNOWN_TYPES 7

/* Add reference to new image type here */
static KnownImageType available_types[] = {
   {png_init, "png"},     /* {name of initialisation function, image subtype} */
   {bmp_init, "bmp"},     /* {name of initialisation function, image subtype} */
   {jpeg_init, "jpeg"},
   {targa_init, "targa"},
   {ppm_init, "x-portable-pixmap"},
   {ppm_init, "x-portable-graymap"},
   {tiff_init, "tiff"}
};

/*             You do not need to modify anything below here                 */
/*****************************************************************************/

/* Necessary includes */
#include <jni.h>
#include "vlc_content_image_ImageDecoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef _WIN32
/* Windoze silliness. */
#include <io.h>
#include <fcntl.h>
#define pipe(fd) (_pipe(fd, 8192, _O_BINARY))
#define write _write
#define read _read
#define lseek _lseek
#define close _close
#define creat _creat
#define unlink _unlink

#else
#include <unistd.h>  /* needed for pipe(2) and write(2) */
#endif

/* Convenience macros */
#define STRSAME(x,y)     (strcmp((x),(y)) == 0)
#define JFREAD(file,buf,sizeofbuf)  \
  ((size_t) fread((void *) (buf), (size_t) 1, (size_t) (sizeofbuf), (file)))
#define ReadOK(file,buffer,len)(JFREAD(file,buffer,len) == ((size_t) (len)))

#define ERROR_LEN 200

/* Macros to deal with unsigned chars as efficiently as compiler allows */
typedef unsigned char U_CHAR;
#define UCH(x)((int) (x))


/* This structure is used to pass data between the general image decoding */
/* routines, and the specifc image decoding routines */
struct param {
   FILE *fptr;                             /* file ptr to encoded data */
   int width;                              /* width of the image */
   int height;                             /* height of the image */
   jint *buffer;                           /* one rows worth of pixels */
   int row_num;                            /* current row number */
   int error;                              /* TRUE on error, FALSE otherwise */
   char error_msg[ERROR_LEN];              /* error message set on error */
   void (*start_input)(Parameters);        /* start function */
   void (*get_pixel_row)(Parameters);      /* get pixel function */
   void (*finish_input)(Parameters);       /* end function */
};

/* Error Strings */

#define ERR_OUT_OF_MEMORY "Insufficient memory"
#define ERR_INPUT_EOF "Premature end of input file"

/* from common.c */
U_CHAR **alloc2DByteArray(int rows, int cols);
void free2DByteArray(U_CHAR **arr);
jint **alloc2DJIntArray(int rows, int cols);
void free2DJIntArray(jint **arr);

#endif /* _DECODE_IMAGE_H */
/******************************************************************************/
