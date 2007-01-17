/*****************************************************************************
 *                     The Virtual Light Company Copyright (c) 1999 - 2000
 *                                         C Source
 *
 * This code is licensed under the GNU Library GPL. Please read license.txt
 * for the full details. A copy of the LGPL may be found at
 *
 * http://www.gnu.org/copyleft/lgpl.html
 *
 * Project:     Image Content Handlers
 * URL:          http://www.vlc.com.au/imageloader/
 *
 ****************************************************************************/

/****************************************************************************/
#include "decode_image.h"
#include <png.h>


/* Private version of data source object */
typedef struct _png_source_struct * png_source_ptr;

typedef struct _png_source_struct {
    struct param pub;                /* public fields */
    int current_row;                 /* current row that we should be returning */
    png_bytep *row_pointers;      /* entire image held in memory */
} png_source_struct;


/*
 * Read one row of pixels.
 * The row of pixel data is copied into params->buffer
 */
static void get_row_gray (Parameters params)
{
    int i;
    png_bytep active_row;
    png_bytep ptr;
    jint *data;

    png_source_ptr source = (png_source_ptr) params;

    data = source->pub.buffer;
    active_row = source->row_pointers[source->current_row];

    ptr = active_row;

    /* put each pixel into the buffer to send back to java */
    for(i = 0; i < source->pub.width; i++) {

        data[i] = (jint)(*ptr++ & 0xff);

/*
This is for 16 bit pixels
        r = ((unsigned int)(*ptr++ >> 3) & 0x1f);
        g = ((unsigned int)(*ptr++ >> 3) & 0x1f);
        b = ((unsigned int)(*ptr++ >> 3) & 0x1f);
        a = ((unsigned int)(*ptr++ == 0)?0:1);

        data[i] = (jshort)((a << 15) + (r << 10) + (g << 5) + b);
*/
    }

    /* free each row as we go */
    free(active_row);
    source->row_pointers[source->current_row] = NULL;

    /* increment our row count */
    source->current_row++;
}

static void get_row_gray_a (Parameters params)
{
    int i;
    png_bytep active_row;
    png_bytep ptr;
    jint *data;
    jint a, g;

    png_source_ptr source = (png_source_ptr) params;

    data = source->pub.buffer;
    active_row = source->row_pointers[source->current_row];

    ptr = active_row;

    /* put each pixel into the buffer to send back to java */
    for(i = 0; i < source->pub.width; i++) {
        /* extract components from byte array and store them */
        /* in the int array which is accessible by caller */
        /* Note that png stores bytes in RGBA format */

        g = (jint)(*ptr++ & 0xff);
        a = (jint)(*ptr++ & 0xff);

/*
This is for 16 bit pixels
        r = ((unsigned int)(*ptr++ >> 3) & 0x1f);
        g = ((unsigned int)(*ptr++ >> 3) & 0x1f);
        b = ((unsigned int)(*ptr++ >> 3) & 0x1f);
        a = ((unsigned int)(*ptr++ == 0)?0:1);
*/

        /* Required to return data in ARGB format */
        data[i] = (a << 8) + g;
/*
        data[i] = (jshort)((a << 15) + (r << 10) + (g << 5) + b);
*/
    }

    /* free each row as we go */
    free(active_row);
    source->row_pointers[source->current_row] = NULL;

    /* increment our row count */
    source->current_row++;
}

static void get_row_rgb (Parameters params)
{
    int i;
    png_bytep active_row;
    png_bytep ptr;
    jint *data;
    jint r, g, b;

    png_source_ptr source = (png_source_ptr) params;

    data = source->pub.buffer;
    active_row = source->row_pointers[source->current_row];

    ptr = active_row;

    /* put each pixel into the buffer to send back to java */
    for(i = 0; i < source->pub.width; i++) {
        /* extract components from byte array and store them */
        /* in the int array which is accessible by caller */
        /* Note that png stores bytes in RGBA format */

        r = (jint)(*ptr++ & 0xff);
        g = (jint)(*ptr++ & 0xff);
        b = (jint)(*ptr++ & 0xff);

/*
This is for 16 bit pixels
        r = ((unsigned int)(*ptr++ >> 3) & 0x1f);
        g = ((unsigned int)(*ptr++ >> 3) & 0x1f);
        b = ((unsigned int)(*ptr++ >> 3) & 0x1f);
        a = ((unsigned int)(*ptr++ == 0)?0:1);
*/

        /* Required to return data in ARGB format */
        data[i] = (r << 16) + (g << 8) + b;
/*
        data[i] = (jshort)((a << 15) + (r << 10) + (g << 5) + b);
*/
    }

    /* free each row as we go */
    free(active_row);
    source->row_pointers[source->current_row] = NULL;

    /* increment our row count */
    source->current_row++;
}

static void get_row_rgba (Parameters params)
{
    int i;
    png_bytep active_row;
    png_bytep ptr;
    jint *data;
    jint a, r, g, b;

    png_source_ptr source = (png_source_ptr) params;

    data = source->pub.buffer;
    active_row = source->row_pointers[source->current_row];

    ptr = active_row;

    /* put each pixel into the buffer to send back to java */
    for(i = 0; i < source->pub.width; i++) {
        /* extract components from byte array and store them */
        /* in the int array which is accessible by caller */
        /* Note that png stores bytes in RGBA format */

        r = (jint)(*ptr++ & 0xff);
        g = (jint)(*ptr++ & 0xff);
        b = (jint)(*ptr++ & 0xff);
        a = (jint)(*ptr++ & 0xff);

/*
This is for 16 bit pixels
        r = ((unsigned int)(*ptr++ >> 3) & 0x1f);
        g = ((unsigned int)(*ptr++ >> 3) & 0x1f);
        b = ((unsigned int)(*ptr++ >> 3) & 0x1f);
        a = ((unsigned int)(*ptr++ == 0)?0:1);
*/

        /* Required to return data in ARGB format */
        data[i] = (a << 24) + (r << 16) + (g << 8) + b;
/*
        data[i] = (jshort)((a << 15) + (r << 10) + (g << 5) + b);
*/
    }

    /* free each row as we go */
    free(active_row);
    source->row_pointers[source->current_row] = NULL;

    /* increment our row count */
    source->current_row++;
}

#define ERREXIT(str) \
                  strncpy(source->pub.error_msg, str, ERROR_LEN); \
                  source->pub.error_msg[ERROR_LEN-1] = '\0'; \
                  source->pub.error = JNI_TRUE; \
                  return;
/*
 * Read the file header; return image size
 */
static void start_input_png (Parameters params)
{
    png_source_ptr source = (png_source_ptr) params;
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    png_uint_32 row, i;
    int fail;

    /* Allocate read structure */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,
                                                (png_error_ptr)NULL, (png_error_ptr)NULL);

    /* couldn't allocate and initialize */
    if (png_ptr == NULL) {
        ERREXIT("Out of memory");
    }

    /* Allocate/initialize the memory for image information. */
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        ERREXIT("Out of memory");
    }

    /* Set error handling if you are using the setjmp/longjmp method (this is
     * the normal method of doing things with libpng).  REQUIRED unless you
     * set up your own error handlers in the png_create_read_struct() earlier.
     */
    if (setjmp(png_ptr->jmpbuf)) {
        /* Free all of the memory associated with the png_ptr and info_ptr */
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        ERREXIT("Error reading input stream");
    }

    /* Set up the input control if you are using standard C streams */
    png_init_io(png_ptr, source->pub.fptr);

    /* The call to png_read_info() gives us all of the information from the
     * PNG file before the first IDAT (image data chunk).  REQUIRED
     */
    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                     &interlace_type, NULL, NULL);

	int has_transparency = 0;

    /* Expand paletted or RGB images with transparency to full alpha channels
     * so the data will be available as RGBA quartets.
     */
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
		has_transparency = 1;

		//Alan: seems to cause crashes
        png_set_expand(png_ptr);

        // Alan: Docs say to do this, but it causes crash as well
		png_set_tRNS_to_alpha(png_ptr);
	}

    /* PNG can have files with 16 bits per channel.  We are only handling */
    /* 8 bits per channel, this will strip the pixels down to 8 bit. */
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    switch (color_type) {
        case PNG_COLOR_TYPE_GRAY:
            if (has_transparency) {
				source->pub.numComponents = 2;
				source->pub.get_pixel_row = get_row_gray_a;
			} else {
				source->pub.numComponents = 1;
				source->pub.get_pixel_row = get_row_gray;
			}

            if(bit_depth < 8)
                png_set_expand(png_ptr);

			// Alan: Causes the decode to crash?
            //png_set_filler(png_ptr, 255, PNG_FILLER_AFTER);
            break;

        case PNG_COLOR_TYPE_GRAY_ALPHA:
            source->pub.numComponents = 2;
            source->pub.get_pixel_row = get_row_gray_a;
            break;

        case PNG_COLOR_TYPE_RGB:
			if (has_transparency) {
	            source->pub.numComponents = 4;
	            source->pub.get_pixel_row = get_row_rgba;
	        } else {
	            source->pub.numComponents = 3;
	            source->pub.get_pixel_row = get_row_rgb;
			}

            break;

        case PNG_COLOR_TYPE_RGB_ALPHA:
            source->pub.numComponents = 4;
            source->pub.get_pixel_row = get_row_rgba;
            break;

        case PNG_COLOR_TYPE_PALETTE:
            source->pub.numComponents = 4;
            source->pub.get_pixel_row = get_row_rgba;

            /* Expand paletted colors into true RGB triplets */
            if (bit_depth <= 8)
                png_set_expand(png_ptr);

            png_set_filler(png_ptr, 255, PNG_FILLER_AFTER);
            break;
    }

	// Alan: Wasn't here,is it needed?
	png_read_update_info(png_ptr, info_ptr);

    /* Allocate the memory to hold the image using the fields of info_ptr. */

    fail = JNI_FALSE;     /* JNI defines TRUE/FALSE so we may as well use if */

    /* Allocate space for row pointers */
    source->row_pointers = (png_bytep *) malloc (height * sizeof(png_bytep));

    /* Allocate the individual rows */
    if (source->row_pointers) {
        row = 0;
        while((!fail && (row < height))) {
            /* allocate one byte for each of the 4 components for this row */
            source->row_pointers[row] =
                (png_bytep) malloc(width * source->pub.numComponents);

            /* check to make sure memory allocation suceeded */
            if (source->row_pointers[row] == NULL) {
                for(i=0; i<row; i++)
                    free(source->row_pointers[i]);
                free(source->row_pointers);

                fail = JNI_TRUE;
            }
            else
                row++;
        }
    }
    else
        fail = JNI_TRUE;

    if (fail) {
        /* Free all of the memory associated with the png_ptr and info_ptr */
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        ERREXIT("Out of memory");
    }

    /* Read the entire image in one go */
    png_read_image(png_ptr, source->row_pointers);

    /* read rest of file, and get additional chunks in info_ptr - REQUIRED */
    png_read_end(png_ptr, info_ptr);

    /* clean up after the read, and free any memory allocated - REQUIRED */
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

    source->current_row = 0;

    /* set image width and height */
    source->pub.width = (int) width;
    source->pub.height = (int) height;
}


/*
 * Finish up at the end of the file.
 */
static void finish_input_png (Parameters params)
{
    png_source_ptr source = (png_source_ptr) params;
    int i;

    /* free allocated memory */

    /* Each row should be freed as it is read, so this is here to */
    /* free memory if an error occurs and each row is not read */
    for(i=0; i<source->pub.height; i++) {
        if (source->row_pointers[i] != NULL)
            free(source->row_pointers[i]);
    }

    free(source->row_pointers);
}


/*
 * Performs initialisation.  This function sets up necessary function pointers
 * and returns a "Parameters object" to that the calling function can access
 * the necessary internal functions of this file.
 */
Parameters png_init ()
{
    png_source_ptr source;

    /* Create module interface object */
    source = (png_source_ptr) malloc(sizeof(png_source_struct));

    if (source != NULL) {
        /* Initialise structure */
        source->pub.fptr = NULL;
        source->pub.width = -1;
        source->pub.height = -1;
        source->pub.numComponents = 3;
        source->pub.buffer = NULL;
        source->pub.row_num = 0;
        source->pub.error = JNI_FALSE;
        source->pub.error_msg[0] = '\0';

        source->current_row = 0;
        source->row_pointers = NULL;

        /* Fill in method ptrs */
        source->pub.start_input = start_input_png;
        source->pub.finish_input = finish_input_png;
    }

    /* return the reference to initialised parameter structure */
    return (Parameters) source;
}

