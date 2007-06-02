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

/*
 * readtiff.c
 *
 * This file contains routines to read input images in Tiff format.
 *
 * The libtiff library does not like to operate with pipes because it performs
 * random seeks into the stream. To deal with this the code makes use of
 * the TIFFClientOpen function and replaces all the normal internal routines
 * with our own custom work. It then takes the entire pipe's contents into an
 * internal buffer which the library can then use to search into.
 */

#include "decode_image.h"
#include <tiffio.h>

#ifdef _WIN32
/* Windoze silliness. */
#include <io.h>
#define read _read

#else
#include <unistd.h>
#endif

/* Error Strings */
#define ERR_TIF_NO_OPEN "Error with tiff internals"
#define ERR_FD_SIZE "File descriptor is too large"

/* buffer size, chosen for typical block size */
#define BUF_SIZE 8192

/* Must be greater than the largest file descriptor that can be opened */
#define MAX_FD 1024

#define ERREXIT(str) { \
                  strncpy(source->pub.error_msg, str, ERROR_LEN); \
                  source->pub.error_msg[ERROR_LEN-1] = '\0'; \
                  source->pub.error = JNI_TRUE; \
                  return; }

#define MIN(x,y)((x)<(y)?(x):(y))

/* buffer used to simulate reading from a file */
typedef struct _file_buffer * file_buffer_ptr;

typedef struct _file_buffer {
   char **rows;               /* pntr to array of rows */
   int num_rows;              /* number of rows */
   int buffer_size;           /* total number of bytes in buffer */
   int current_row;           /* current row */
   int current_offset;        /* current offset in the current row */
} file_buffer;

/* Private version of data source object */
typedef struct _tiff_source_struct * tiff_source_ptr;

typedef struct _tiff_source_struct {
  struct param pub;                /* public fields */

  int current_row;                 /* current row that we should be returning */
  uint32 *raster;                  /* contains all the image data in ARGB format */
} tiff_source_struct;

/* static list of buffers */
static file_buffer_ptr buffers[MAX_FD];

static int initialized = 0;

/* Function forward decls */
static tsize_t pipeRead(thandle_t fd, tdata_t buf, tsize_t size);
static tsize_t pipeWrite(thandle_t fd, tdata_t buf, tsize_t size);
static toff_t pipeSeek(thandle_t fd, toff_t off, int whence);
static int pipeClose(thandle_t fd);
static toff_t pipeSize(thandle_t fd);
static void buffer_file(int fd);
static int pipeMMap(thandle_t fd, tdata_t* pbase, toff_t* psize);
static void pipeUnMMap(thandle_t fd, tdata_t base, toff_t size);

/*
 * Read one row of pixels.
 * This version is for reading 8-bit colormap indexes
 */
static void get_row_rgba(Parameters params)
{
    tiff_source_ptr source = (tiff_source_ptr)params;
    unsigned int i;
    uint32 tmp;
    register uint32 *inptr;
    jint *data;
    jint a, r, g, b;

    inptr = source->raster + source->pub.width * source->current_row;
    data = source->pub.buffer;

    /* put each pixel into the buffer to send back to java */
    for(i = 0; i < source->pub.width; i++)
    {
        /* extract components from byte array and store them */
        /* in the int array which is accessible by caller */
        /* Note that tiff stores bytes in ARGB format */
        tmp = *inptr++;

        a = (jint)TIFFGetA(tmp);
        r = (jint)TIFFGetR(tmp);
        g = (jint)TIFFGetG(tmp);
        b = (jint)TIFFGetB(tmp);

        data[i] = (a << 24) +(r << 16) +(g << 8) + b;
    }

    source->current_row--;
}

static void get_row_rgb(Parameters params)
{
    tiff_source_ptr source = (tiff_source_ptr)params;
    unsigned int i;
    uint32 tmp;
    register uint32 *inptr;
    jint *data;
    jint r, g, b;

    inptr = source->raster + source->pub.width * source->current_row;
    data = source->pub.buffer;

    /* put each pixel into the buffer to send back to java */
    for(i = 0; i < source->pub.width; i++)
    {
        /* extract components from byte array and store them */
        /* in the int array which is accessible by caller */
        /* Note that tiff stores bytes in ARGB format */
        tmp = *inptr++;

        r = (jint)TIFFGetR(tmp);
        g = (jint)TIFFGetG(tmp);
        b = (jint)TIFFGetB(tmp);

        data[i] = (r << 16) +(g << 8) + b;

    }

    source->current_row--;
}

static void get_row_gray(Parameters params)
{
    tiff_source_ptr source = (tiff_source_ptr)params;
    unsigned int i;
    uint32 tmp;
    register uint32 *inptr;
    jint *data;

    inptr = source->raster + source->pub.width * source->current_row;
    data = source->pub.buffer;

    /* put each pixel into the buffer to send back to java */
    for(i = 0; i < source->pub.width; i++)
    {
        /* Note that tiff stores bytes in ARGB format */
        tmp = *inptr++;

        data[i] = (jint)(tmp & 0xff);
    }

    source->current_row--;
}

/*
 * Read the file header; return image size and component count.
 */
static void start_input_tiff(Parameters params)
{
    tiff_source_ptr source = (tiff_source_ptr) params;
    uint32 w, h;
    uint32 components = 0;
    uint32 type = 0;
    size_t npixels;
    TIFF *tif;
    int fd;

    /* open file with tiff fdopen */
    fd = fileno(source->pub.fptr);
    if(fd >= MAX_FD)
       ERREXIT(ERR_FD_SIZE);

    /* read in the file into memory */
    buffer_file(fd);

    tif = TIFFClientOpen("imagefile",
                         "rm",
                        (thandle_t)fd,
                        pipeRead,
                        pipeWrite,
                        pipeSeek,
                        pipeClose,
                        pipeSize,
                        pipeMMap,
                        pipeUnMMap);

    if(tif)
    {
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
        TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &type);
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &components);

        switch(type)
        {
            case PHOTOMETRIC_MINISWHITE:
            case PHOTOMETRIC_MINISBLACK:
            case PHOTOMETRIC_MASK:
                source->pub.numComponents = 1;
                source->pub.get_pixel_row = get_row_gray;
                break;

            case PHOTOMETRIC_RGB:
            case PHOTOMETRIC_SEPARATED:
            case PHOTOMETRIC_YCBCR:
            case PHOTOMETRIC_CIELAB:
/*
    NOTE: Comment out to allow to fall through to 4 component color. Needed
    because there's some odd bug with Sun's JPEG encoder when they are deal
    with 3 component rasters generated by this output. If the image uses 4
    components, it works fine, but not with 3 where it seems to swap the red
    and blue components(almost like BGR, rather than RGB) :(
*/
/*
                source->pub.numComponents = 3;
                source->pub.get_pixel_row = get_row_rgb;
                break;
*/
            case PHOTOMETRIC_PALETTE:
                source->pub.numComponents = 4;
                source->pub.get_pixel_row = get_row_rgba;
                break;
        }

        /* how many pixels in array? */
        npixels = w * h;
        source->raster = (uint32*) _TIFFmalloc(npixels * sizeof(uint32));

        if(source->raster != NULL)
        {
            if(!TIFFReadRGBAImage(tif, w, h, source->raster, 0))
            {
                /* do error message here */
                ;
            }
        }
        else
        {
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

static void finish_input_tiff(Parameters params)
{
    tiff_source_ptr source = (tiff_source_ptr) params;

    _TIFFfree(source->raster);
}

/*
 * Performs initialisation.  This function sets up necessary function pointers
 * and returns a "Parameters object" to that the calling function can access
 * the necessary internal functions of this file.
 */
Parameters tiff_init()
{
    tiff_source_ptr source;

    if(!initialized)
    {
        int i;
        for(i = 0; i < MAX_FD; i++)
            buffers[i] = NULL;

        initialized = 1;
    }

    /* Create module interface object */
    source = (tiff_source_ptr) malloc(sizeof(tiff_source_struct));

    if(source != NULL)
    {
        /* Initialise structure */
        source->pub.fptr = NULL;
        source->pub.width = -1;
        source->pub.height = -1;
        source->pub.numComponents = 4;
        source->pub.buffer = NULL;
        source->pub.row_num = 0;
        source->pub.error = JNI_FALSE;
        source->pub.error_msg[0] = '\0';

        source->current_row = 0;
        source->raster = NULL;

        /* Fill in method ptrs */
        source->pub.start_input = start_input_tiff;
        source->pub.get_pixel_row = get_row_rgba;
        source->pub.finish_input = finish_input_tiff;
    }

    /* return the reference to initialised parameter structure */
    return(Parameters) source;
}

void tiffErrorHandler(const char* module, const char* fmt, va_list ap)
{
     if(module != NULL)
          fprintf(stderr, "%s: ", module);
     fprintf(stderr, "Error, ");
     vfprintf(stderr, fmt, ap);
     fprintf(stderr, ".\n");
}

void tiffWarningHandler(const char* module, const char* fmt, va_list ap)
{
     if(module != NULL)
          fprintf(stderr, "%s: ", module);
     fprintf(stderr, "Warning, ");
     vfprintf(stderr, fmt, ap);
     fprintf(stderr, ".\n");
}

/*
 * Reads the number of given bytes from the buffer.
 * Behaves like read(2)
 */
static tsize_t pipeRead(thandle_t fd, tdata_t buf, tsize_t size)
{
    file_buffer_ptr ptr;
    int bytes_left, bytes_on_line, bytes_to_read, total, overflow;
    char *inptr, *outptr;

    ptr = buffers[(int)fd];

    total = size;
    bytes_left = size;
    outptr = (char *) buf;

    do
    {
        bytes_on_line = BUF_SIZE - ptr->current_offset;
        bytes_to_read = MIN(bytes_left, bytes_on_line);

        /* overflow is used to see if we try to read more bytes than exist */
        overflow =  ptr->current_row * BUF_SIZE +
                        ptr->current_offset + bytes_to_read - ptr->buffer_size;

        if(overflow > 0)
        {
            total -= overflow;
            bytes_to_read -= overflow;
            bytes_left = bytes_to_read;
        }

        /* copy the data into output buffer */
        inptr = ptr->rows[ptr->current_row] + ptr->current_offset;
        memcpy(outptr, inptr, bytes_to_read);
        ptr->current_offset += bytes_to_read;
        bytes_left -= bytes_to_read;

        if(bytes_left > 0)
        {
            /* move to next row in buffer */
            outptr += bytes_to_read;
            ptr->current_row++;
            ptr->current_offset = 0;
        }

    } while(bytes_left > 0);

    return(tsize_t) total;
}

/*
 * Writes data into the buffer.
 * Not implemented for our reader.
 */
static tsize_t pipeWrite(thandle_t fd, tdata_t buf, tsize_t size)
{
    tiffErrorHandler("pipeWrite",
                        "Writing of tiff images not supported", NULL);
    return(tsize_t) -1;
}

/*
 * Seeks to the given position in the buffer.
 * Behaves like lseek(2)
 */
static toff_t pipeSeek(thandle_t fd, toff_t off, int whence)
{
    file_buffer_ptr ptr;
    int ret_val;
    int seek_off;

    ptr = buffers[(int)fd];

    /* calculate absolute offset in buffer */
    if(whence == 0)
        seek_off = off;
    else if(whence == 1)
        seek_off = ptr->current_row * BUF_SIZE + ptr->current_offset + off;
    else if(whence == 2)
        seek_off = ptr->buffer_size + off;
    else
    {
        tiffWarningHandler("pipeSeek", "invalid whence", NULL);
        seek_off = -1;
    }

    /* ensure offset is valid */
    if(seek_off < 0 || seek_off >= ptr->buffer_size)
        ret_val = -1;
    else
        ret_val = seek_off;

    if(ret_val != -1)
    {
        /* now seek! */
        ptr->current_row = seek_off / BUF_SIZE;
        ptr->current_offset = seek_off % BUF_SIZE;
    }

    return(toff_t) ret_val;
}

/*
 * Frees the memory allocated to the internal buffers in
 * file buffer.
 */
static void free_buffer(file_buffer_ptr ptr)
{
    int i;

    /* free each of the rows */
    for(i = 0; i < ptr->num_rows; i++)
        free(ptr->rows[i]);

    /* free the array of row pointers */
    free(ptr->rows);
}

/*
 * Assume that this is a cleanup function.
 * This releases memory allocated to our buffer.
 * File descriptor is left alone.
 */
static int pipeClose(thandle_t fd)
{
    file_buffer_ptr ptr = buffers[(int)fd];

    /* assume that the file descriptor will be closed elsewhere */

    /* however, free memory as it is assumed that this is a general */
    /* cleanup function */
    free_buffer(ptr);
    free(ptr);
    buffers[(int)fd] = NULL;

    return 0;
}

/*
 * Returns size of the buffer
 */
static toff_t pipeSize(thandle_t fd)
{
    file_buffer_ptr ptr = buffers[(int)fd];
    return ptr->buffer_size;
}

/*
 * This fuction will attempt to fill the current buffer of the
 * file_buffer_ptr structure, from the input file descriptor.
 * Returns 1 on EOF, 0 otherwise
 * Called by buffer_file()
 */
static int read_one_buffer(int fd, file_buffer_ptr ptr)
{
    char *buf_ptr;
    int num_read;
    int ret_val;

    ret_val = 0;
    buf_ptr = ptr->rows[ptr->current_row] + ptr->current_offset;
    num_read = read(fd, buf_ptr,(BUF_SIZE - ptr->current_offset));

    if(num_read == 0)
    {
        /* we have reached EOF */
        ret_val = 1;
    }
    else if(num_read == -1)
    {
        /* error occured reading! */
        tiffErrorHandler("read_one_buffer",
                         "Error reading from file descriptor",
                         NULL);
        /* free any memory allocated */
        free_buffer(ptr);
        ret_val = 1;
    }
    else
    {
        /* everything is okay */
        ptr->buffer_size += num_read;
        ptr->current_offset += num_read;
        if(ptr->current_offset >= BUF_SIZE)
        {
            ptr->current_offset = 0;
            ptr->current_row++;
        }
    }

    /* return whether we are at EOF or not */
    return ret_val;
}

/*
 * Reads all the data from the file descriptor into a temporary
 * buffer in memory.  This is needed in case the file descriptor
 * is a stream, where we can not seek backwards
 */
static void buffer_file(int fd)
{
    file_buffer_ptr ptr;
    int done;
    char **tmp_ptr;

    if(buffers[fd])
        printf("pre-allocated buffer!\n");

    buffers[fd] = (file_buffer_ptr) malloc(sizeof(file_buffer));
    ptr = buffers[fd];

    if(ptr)
    {
        /* initialise structure */
        ptr->rows = NULL;
        ptr->num_rows = 0;
        ptr->buffer_size = 0;
        ptr->current_row = 0;
        ptr->current_offset = 0;

        /* init flag */
        done = 0;

        do
        {
            if(ptr->current_offset != 0)
            {
                /* our last read did not fill up the buffer */
                /* so attempt to fill the buffer now */
                done = read_one_buffer(fd, ptr);
            }
            else
            {
                /* we are reading a new row into our buffer */

                if(ptr->current_row >= ptr->num_rows)
                {
                    /* need to re allocate row pntrs */
                    /* use malloc and memcpy as m$ realloc seems to be acting up */
                    tmp_ptr = malloc((ptr->current_row+1)*sizeof(char *));
                    if(tmp_ptr)
                    {
                        memcpy(tmp_ptr, ptr->rows, ptr->current_row * sizeof(char *));
                        free(ptr->rows);

                        ptr->rows = tmp_ptr;
                        ptr->num_rows = ptr->current_row+1;
                    }
                    else
                    {
                        /* not enough memory */
                        tiffErrorHandler("buffer_file", "Out of memory", NULL);
                        free_buffer(ptr);
                        done = 1;
                    }
                }

                /* allocate memory for the new row */
                ptr->rows[ptr->current_row] = (char *)malloc(BUF_SIZE * sizeof(char));
                if(!ptr->rows[ptr->current_row])
                {
                    /* not enough memory */
                    tiffErrorHandler("buffer_file", "Out of memory", NULL);
                    free_buffer(ptr);
                    done = 1;
                }

                /* attempt to fill this newly allocated row */
                if(!done)
                    done = read_one_buffer(fd, ptr);
            }
        } while(!done);

        /* have finished reading so set file pointer to beginning */
        ptr->current_row = 0;
        ptr->current_offset = 0;
    }
    else
    {
        /* not enough memory */
        tiffErrorHandler("buffer_file", "Out of memory", NULL);
    }
}

static int pipeMMap(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
    (void) fd;(void) pbase;(void) psize;
     return(0);
}

static void pipeUnMMap(thandle_t fd, tdata_t base, toff_t size)
{
    (void) fd;(void) base;(void) size;
}
