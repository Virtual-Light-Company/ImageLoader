/*****************************************************************************
 *                     The Virtual Light Company Copyright(c) 1999 - 2000
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
#include <jpeglib.h>

#define ERR_COLOR_SPACE "Unsupported colorspace"

/* Our own custom error handler */
struct my_error_mgr {
    struct jpeg_error_mgr pub;    /* "public" fields of error handler*/
    Parameters param;                /* pointer to Parameter struct */
};

/* Private version of error handler */
typedef struct my_error_mgr * my_error_ptr;

/* Private version of data source object */
typedef struct _jpeg_source_struct * jpeg_source_ptr;

typedef struct _jpeg_source_struct {
    struct param pub;                /* public fields */
    JSAMPLE *image_buffer;         /* Points to large array of R,G,B-order data */
    struct jpeg_decompress_struct cinfo;
    JSAMPARRAY buffer;              /* Output row buffer */
    my_error_ptr err;                /* Our error handler */
} jpeg_source_struct;



/*
 * Here's the routine that will replace the standard error_exit method:
 */
static void my_error_exit(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];

    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    my_error_ptr myerr = (my_error_ptr) cinfo->err;

    /* get the error message into our buffer */
   (*cinfo->err->format_message)(cinfo, buffer);

    /* Now set our error message */
    myerr->param->error = JNI_TRUE;
    strncpy(myerr->param->error_msg, buffer, ERROR_LEN);

    /* ensure string is NULL terminated */
    myerr->param->error_msg[ERROR_LEN-1] = '\0';
}


/*
 * Read one row of pixels.
 * The row of pixel data is copied into params->buffer
 */
static void get_row_jpeg(Parameters params)
{
    jpeg_source_ptr source = (jpeg_source_ptr) params;
    JSAMPLE *ptr;
    int i;
    jint *data;
    jint r, g, b;

    /* Extract one scan line from the jpeg library */
   (void) jpeg_read_scanlines(&(source->cinfo),(source->buffer), 1);

    /* Continue if no errors */
    if(!source->pub.error)
    {
        ptr = source->buffer[0];
        data = source->pub.buffer;

        /* put each pixel into the buffer to send back to java */
        /* extract components from JSAMPLE array and store them */
        /* in the int array which is accessible by caller */
        switch(source->cinfo.output_components)
        {
            case 3:
                for(i = 0; i < source->pub.width; i++)
                {
                    r = (jint)(*ptr++ & 0xff);
                    g = (jint)(*ptr++ & 0xff);
                    b = (jint)(*ptr++ & 0xff);

                    /* Required to return data in RGB format */
                    data[i] = (r << 16) +(g << 8) + b;
                }
                break;

            case 1:
                for(i = 0; i < source->pub.width; i++)
                {
                    r = (jint)(*ptr++ & 0xff);
                    g = r;
                    b = r;

                    /* Required to return data in RGB format */
                    data[i] = (r << 16) +(g << 8) + b;
                }
                break;

            default:
                /* Don't understand this color space */
                strncpy(source->pub.error_msg, ERR_COLOR_SPACE, ERROR_LEN);
                source->pub.error_msg[ERROR_LEN-1] = '\0';
                source->pub.error = JNI_TRUE;
        }
    }
}

/*
 * Read the file header; return image size
 */
static void start_input_jpeg(Parameters params)
{
    jpeg_source_ptr source = (jpeg_source_ptr) params;
    int row_stride;                     /* physical row width in output buffer */

    /* allocate memory for our error handler */
    source->err = (struct my_error_mgr *)malloc(sizeof(struct my_error_mgr));

    if(source->err)
    {
        /* allocate and initialize JPEG decompression object */

        /* We set up the normal JPEG error routines, then override error_exit. */
        source->cinfo.err = jpeg_std_error(&source->err->pub);
        source->err->pub.error_exit = my_error_exit;
        source->err->param = params;

        /* Now we can initialize the JPEG decompression object. */
        jpeg_create_decompress(&(source->cinfo));
        if(source->pub.error)
            goto end;

        /* specify data source */
        jpeg_stdio_src(&(source->cinfo), source->pub.fptr);
        if(source->pub.error)
            goto end;

        /* read file parameters with jpeg_read_header() */
       (void) jpeg_read_header(&(source->cinfo), TRUE);
        if(source->pub.error)
            goto end;

        /* set parameters for decompression */
        /* - no parameters to set */

        /* Start decompressor */
       (void) jpeg_start_decompress(&(source->cinfo));
        if(source->pub.error)
            goto end;

        /* JSAMPLEs per row in output buffer */
        row_stride = source->cinfo.output_width * source->cinfo.output_components;
        /* Make a one-row-high sample array that will go away when */
        /* done with image */
        source->buffer = (*(source->cinfo.mem->alloc_sarray))
         ((j_common_ptr) &(source->cinfo), JPOOL_IMAGE, row_stride, 1);
        if(source->pub.error)
            goto end;

        /* set image width and height */
        source->pub.width = (int) source->cinfo.output_width;
        source->pub.height = (int) source->cinfo.output_height;
		source->pub.numComponents = source->cinfo.output_components;
    }
    else
    {
        /* Our malloc failed, hopefully we never get to here */
        strncpy(source->pub.error_msg, ERR_OUT_OF_MEMORY, ERROR_LEN);
        source->pub.error_msg[ERROR_LEN-1] = '\0';
        source->pub.error = JNI_TRUE;
    }
end:
    /* the label here is for exiting on errors */
    ;
}


/*
 * Finish up at the end of the file.
 */
static void finish_input_jpeg(Parameters params)
{
    jpeg_source_ptr source = (jpeg_source_ptr) params;

    /* Finish decompression */
   (void) jpeg_finish_decompress(&(source->cinfo));

    /* Release JPEG decompression object */
    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&(source->cinfo));

    /* Free memory allocated to the error handler */
    free(source->err);
}


/*
 * Performs initialisation.  This function sets up necessary function pointers
 * and returns a "Parameters object" to that the calling function can access
 * the necessary internal functions of this file.
 */
Parameters jpeg_init()
{
    jpeg_source_ptr source;

    /* Create module interface object */
    source = (jpeg_source_ptr) malloc(sizeof(jpeg_source_struct));

    if(source != NULL)
    {
        /* Initialise structure */
        source->pub.fptr = NULL;
        source->pub.width = -1;
        source->pub.height = -1;
        source->pub.numComponents = 3;
        source->pub.buffer = NULL;
        source->pub.row_num = 0;
        source->pub.error = JNI_FALSE;
        source->pub.error_msg[0] = '\0';

        source->image_buffer = NULL;

        /* Fill in method ptrs */
        source->pub.start_input = start_input_jpeg;
        source->pub.get_pixel_row = get_row_jpeg;
        source->pub.finish_input = finish_input_jpeg;
    }

    /* return the reference to initialised parameter structure */
    return(Parameters) source;
}
