/*****************************************************************************
 *                      ADI Limited Copyright (c) 1998
 *                                C Source
 *
 * Project:    SRCSS
 *             Special Recovery Command Support System.
 *
 * Version History
 * Date        TR/IWOR  Version  Programmer
 * ----------  -------  -------  ------------------------------------------
 * 30/11/1998           1.00     Dion Mendel
 * File created
 *
 ****************************************************************************/

/*
 * This file is loosely based on tif_unix.c from the Tiff library.
 *
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 */

/*
 * These stub functions are used by the Tiff library to provide the
 * functionality of basic file operations: read(2), write(2), lseek(2), etc.
 *
 * The image data for the tiff library comes from a pipe, which means that
 * we cannot seek backwards.  The Tiff library requires seeking backwards
 * to be able to decode images.  The functions in this file read in all
 * the data from the pipe into an internal buffer.  Then any reading or
 * seeking is done on the buffer itself.  This solves the problem of seeking
 * backwards.
 */

/*
 * TIFF Library Custom Routines.
 */
#include "tiffiop.h"
#include <sys/types.h>
#include <stdlib.h>

#ifdef _WIN32
/* Windoze silliness. */
#include <io.h>
#define read _read

#else
#include <unistd.h>
#endif

/* buffer size, chosen for typical block size */
#define BUF_SIZE 8192

#define MIN(x,y) ((x)<(y)?(x):(y))

/* buffer used to simulate reading from a file */
typedef struct _file_buffer * file_buffer_ptr;

typedef struct _file_buffer {
   char **rows;               /* pntr to array of rows */
   int num_rows;              /* number of rows */
   int buffer_size;           /* total number of bytes in buffer */
   int current_row;           /* current row */
   int current_offset;        /* current offset in the current row */
} file_buffer;

/* this must be greater than the largest file descriptor */
/* that can be openned on the system */
#define MAX_FD 1024

/* static list of buffers */
static file_buffer_ptr buffers[MAX_FD];

static void
myErrorHandler(const char* module, const char* fmt, va_list ap)
{
        if (module != NULL)
                fprintf(stderr, "%s: ", module);
        fprintf(stderr, "Error, ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, ".\n");
}

static void
myWarningHandler(const char* module, const char* fmt, va_list ap)
{
        if (module != NULL)
                fprintf(stderr, "%s: ", module);
        fprintf(stderr, "Warning, ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, ".\n");
}

/*
 * Reads the number of given bytes from the buffer.
 * Behaves like read(2)
 */
static tsize_t
_tiffReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
   file_buffer_ptr ptr;
   int bytes_left, bytes_on_line, bytes_to_read, total, overflow;
   char *inptr, *outptr;

   ptr = buffers[(int)fd];

   total = size;
   bytes_left = size;
   outptr = (char *) buf;

   do {
      bytes_on_line = BUF_SIZE - ptr->current_offset;
      bytes_to_read = MIN(bytes_left, bytes_on_line);

      /* overflow is used to see if we try to read more bytes than exist */
      overflow =  ptr->current_row * BUF_SIZE +
                  ptr->current_offset + bytes_to_read - ptr->buffer_size;

      if (overflow > 0) {
         total -= overflow;
         bytes_to_read -= overflow;
         bytes_left = bytes_to_read;
      }

      /* copy the data into output buffer */
      inptr = ptr->rows[ptr->current_row] + ptr->current_offset;
      memcpy(outptr, inptr, bytes_to_read);
      ptr->current_offset += bytes_to_read;
      bytes_left -= bytes_to_read;

      if (bytes_left > 0) {
         /* move to next row in buffer */
         outptr += bytes_to_read;
         ptr->current_row++;
         ptr->current_offset = 0;
      }

   } while(bytes_left > 0);
   
   return (tsize_t) total;
}

/*
 * Writes data into the buffer.
 * Not implemented for our reader.
 */
static tsize_t
_tiffWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
   myErrorHandler("_tiffWriteProc",
                  "Writing of tiff images not supported", NULL);
   return (tsize_t) -1;
}

/*
 * Seeks to the given position in the buffer.
 * Behaves like lseek(2)
 */
static toff_t
_tiffSeekProc(thandle_t fd, toff_t off, int whence)
{
   file_buffer_ptr ptr;
   int ret_val;
   int seek_off;

   ptr = buffers[(int)fd];

   /* calculate absolute offset in buffer */
   if (whence == 0)
      seek_off = off;
   else if (whence == 1)
      seek_off = ptr->current_row * BUF_SIZE + ptr->current_offset + off;
   else if (whence == 2)
      seek_off = ptr->buffer_size + off;
   else {
      myWarningHandler("_tiffSeekProc", "invalid whence", NULL);
      seek_off = -1;
   }

   /* ensure offset is valid */
   if (seek_off < 0 || seek_off >= ptr->buffer_size)
      ret_val = -1;
   else
      ret_val = seek_off;

   if (ret_val != -1) {
      /* now seek! */
      ptr->current_row = seek_off / BUF_SIZE;
      ptr->current_offset = seek_off % BUF_SIZE;
   }

   return (toff_t) ret_val;
}

/*
 * Frees the memory allocated to the internal buffers in
 * file buffer.
 */
static void free_buffer(file_buffer_ptr ptr)
{
   int i;

   /* free each of the rows */
   for(i=0; i<ptr->num_rows; i++) {
      free(ptr->rows[i]);
   }

   /* free the array of row pointers */
   free(ptr->rows);
}

/*
 * Assume that this is a cleanup function.
 * This releases memory allocated to our buffer.
 * File descriptor is left alone.
 */
static int
_tiffCloseProc(thandle_t fd)
{
   file_buffer_ptr ptr;
   ptr = buffers[(int)fd];

   /* assume that the file descriptor will be closed elsewhere */

   /* however, free memory as it is assumed that this is a general */
   /* cleanup function */
   free_buffer(ptr);
   free(ptr);

   return 0;
}

/*
 * Returns size of the buffer
 */
static toff_t
_tiffSizeProc(thandle_t fd)
{
   file_buffer_ptr ptr;
   ptr = buffers[(int)fd];
   return ptr->buffer_size;
}

static int
_tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
        (void) fd; (void) pbase; (void) psize;
        return (0);
}

static void
_tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
        (void) fd; (void) base; (void) size;
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
   num_read = read(fd, buf_ptr, (BUF_SIZE - ptr->current_offset));

   if (num_read == 0) {
      /* we have reached EOF */
      ret_val = 1;
   }
   else if (num_read == -1) {
      /* error occured reading! */
      myErrorHandler("read_one_buffer",
                     "Error reading from file descriptor", NULL);
      /* free any memory allocated */
      free_buffer(ptr);
      ret_val = 1;
   }
   else {
      /* everything is okay */
      ptr->buffer_size += num_read;
      ptr->current_offset += num_read;
      if (ptr->current_offset >= BUF_SIZE) {
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

   buffers[fd] = (file_buffer_ptr) malloc(sizeof(file_buffer));

   ptr = buffers[fd];

   if (ptr) {
      /* initialise structure */
      ptr->rows = NULL;
      ptr->num_rows = 0;
      ptr->buffer_size = 0;
      ptr->current_row = 0;
      ptr->current_offset = 0;

      /* init flag */
      done = 0;

      do {
         if (ptr->current_offset != 0) {
            /* our last read did not fill up the buffer */
            /* so attempt to fill the buffer now */
            done = read_one_buffer(fd, ptr);
         }
         else {
            /* we are reading a new row into our buffer */

            if (ptr->current_row >= ptr->num_rows) {
               /* need to re allocate row pntrs */
               /* use malloc and memcpy as m$ realloc seems to be acting up */
               tmp_ptr = malloc((ptr->current_row+1)*sizeof(char *));
               if (tmp_ptr) {
                  memcpy(tmp_ptr, ptr->rows, ptr->current_row * sizeof(char *));
                  ptr->rows = tmp_ptr;
                  ptr->num_rows = ptr->current_row+1;
               }
               else {
                  /* not enough memory */
                  myErrorHandler("buffer_file", "Out of memory", NULL);
                  free_buffer(ptr);
                  done = 1;
               }
            }

            /* allocate memory for the new row */
            ptr->rows[ptr->current_row] = (char *) malloc(BUF_SIZE * sizeof(char));
            if (!ptr->rows[ptr->current_row]) {
               /* not enough memory */
               myErrorHandler("buffer_file", "Out of memory", NULL);
               free_buffer(ptr);
               done = 1;
            }

            /* attempt to fill this newly allocated row */
            if (!done)
               done = read_one_buffer(fd, ptr);
         }
      } while(!done);

      /* have finished reading so set file pointer to beginning */
      ptr->current_row = 0;
      ptr->current_offset = 0;
   }
   else {
      /* not enough memory */
      myErrorHandler("buffer_file", "Out of memory", NULL);
   }
}

/*
 * Open a TIFF file descriptor for read/writing.
 */
TIFF*
TIFFFdOpen(int fd, const char* name, const char* mode)
{
        TIFF* tif;

        if (fd >= MAX_FD) {
           myErrorHandler("TIFFFdOpen", "fd is too large", NULL);
           return (TIFF*) NULL;
        }

        /* read in the file into memory */
        buffer_file(fd);

        tif = TIFFClientOpen(name, mode,
            (thandle_t) fd,
            _tiffReadProc, _tiffWriteProc,
            _tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
            _tiffMapProc, _tiffUnmapProc);
        if (tif)
                tif->tif_fd = fd;
        return (tif);
}

/*
 * Open a TIFF file for read/writing.
 */
TIFF*
TIFFOpen(const char* name, const char* mode)
{
   myErrorHandler("TIFFOpen", "only TIFFFdOpen supported", NULL);

   return (TIFF*) NULL;
}

void*
_TIFFmalloc(tsize_t s)
{
        return (malloc((size_t) s));
}

void
_TIFFfree(tdata_t p)
{
        free(p);
}

void*
_TIFFrealloc(tdata_t p, tsize_t s)
{
        return (realloc(p, (size_t) s));
}

void
_TIFFmemset(tdata_t p, int v, tsize_t c)
{
        memset(p, v, (size_t) c);
}

void
_TIFFmemcpy(tdata_t d, const tdata_t s, tsize_t c)
{
        memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const tdata_t p1, const tdata_t p2, tsize_t c)
{
        return (memcmp(p1, p2, (size_t) c));
}

TIFFErrorHandler _TIFFwarningHandler = myWarningHandler;
TIFFErrorHandler _TIFFerrorHandler = myErrorHandler;
