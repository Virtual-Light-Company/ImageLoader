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


/****************************************************************************\
This is the driver for the image decoder library.  If you are looking at
adding a new image format, you do not need to modify this file.
To add a new image format, copy one of the existing image format files
e.g. readpng.c, and make appropriate modifications to the header of
decode_image.h
\****************************************************************************/

#include "decode_image.h"

/* Has this library been initialised yet or not? */
static int init = JNI_FALSE;

/* This is necessary for keeping track of the current state in between */
/* function calls */
static Parameters *param_list;

/* This a list of pipes that are used for transfering the data to be */
/* decoded, from the java side to the decoding modules */
static int **fd_list;

/*
 * Private function.  This provides a convenience function for throwing
 * exceptions back to the java calling method.
 * param: env       - standard JNI env pointer
 *        exception - the exception class e.g. "java/lang/exception"
 *        message   - reason why exception occured
 */
static void throw_exception(JNIEnv *env, char *exception, char *message)
{
   jclass newExcCls;

   (*env)->ExceptionDescribe(env);
   (*env)->ExceptionClear(env);

   newExcCls = (*env)->FindClass(env, exception);
   if (newExcCls == 0) { /* Unable to find the new exception class, give up. */
       return;
   }

   if (message == NULL)
      (*env)->ThrowNew(env, newExcCls, "");
   else
      (*env)->ThrowNew(env, newExcCls, message);
}


/*
 * Desc:      Returns an array of strings containing the support file formats
 *            These strings are the mime subtype for the
 *            image format. e.g. "gif", "png", "tiff"
 * Input:
 *            None
 * Output:
 *            None
 * Return:
 *            Array of strings containing supported image subtypes.
 * Exception:
 *            None
 * Class:     vlc_net_content_image_ImageDecoder
 * Method:    getFileFormats
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL
Java_vlc_net_content_image_ImageDecoder_getFileFormats
(JNIEnv *env, jobject obj)
{
   int i;
   jarray string_array;

   /* create an array of strings and initialise it to contain NULL objects */
   string_array = (*env)->NewObjectArray(env, NUM_KNOWN_TYPES,
                             (*env)->FindClass(env, "java/lang/String"), NULL);

   /* now fill the array with string objects representing the allowed */
   /* image types */
   for(i=0; i<NUM_KNOWN_TYPES; i++) {
      (*env)->SetObjectArrayElement(env, string_array, i,
                   (*env)->NewStringUTF(env, available_types[i].type_string));
   }

   return string_array;
}

/*
 * Desc:      Performs initialisation for the library.  This function is
 *            only to be called once.
 * Input:
 *            num_threads: maximum number of threads that will access this
 *                         library at any point in time.
 * Output:
 *            None
 * Return:
 *            None
 * Exception:
 *            java.lang.InternalError if this function is invoked more than
 *            once, or if the 'num_threads' parameter is illegal
 * Class:     vlc_net_content_image_ImageDecoder
 * Method:    initialize
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_vlc_net_content_image_ImageDecoder_initialize
(JNIEnv *env, jobject obj, jint num_threads)
{
   char buf[100];
   int row, i;
   int fail;

   /* ensure we are only called once */
   if (init) {
      throw_exception(env, "java/lang/InternalError",
                           "Initialize called more than once");
   }

   init = JNI_TRUE;

   /* ensure that the number of threads is valid */
   if (num_threads <= 0) {
      sprintf(buf, "Illegal number of threads: %d", num_threads);
      throw_exception(env, "java/lang/InternalError", buf);
   }

   /* Allocate memory for structures that exist for the entire life of the */
   /* library.  This memory is never freed */

   /* allocate memory for num_threads lots of params */
   param_list = (Parameters *) calloc(num_threads, sizeof(Parameters));
   if (!param_list) {
      /* No memory?, hopefully we'll never see this */
      throw_exception(env, "java/lang/OutOfMemoryError", NULL);
   }

   /* allocate memory for num_threads lots of pipe structures */
   fd_list = (int **) malloc(num_threads * sizeof(int *));

   /* Allocate the individual rows */
   if (fd_list) {
      fail = JNI_FALSE;
      row = 0;
      while((!fail && (row < num_threads))) {
         fd_list[row] = (int *) malloc(2 * sizeof(int));

         /* check to make sure memory allocation suceeded */
         if (fd_list[row] == NULL) {
            for(i=0; i<row; i++)
               free(fd_list[i]);
            free(fd_list);

            fail = JNI_TRUE;
         }
         else
            row++;
      }
   }
   else
      fail = JNI_TRUE;

   if (fail)
      fd_list = NULL;

   if (!fd_list) {
      /* No memory?, hopefully we'll never see this */
      throw_exception(env, "java/lang/OutOfMemoryError", NULL);
   }
}

/*
 * Desc:      Prepares the decoder library for the imminent decoding of an
 *            image.  This function sets the type of the image format that
 *            the library is to decode, and whether the library should
 *            use temporary files when decoding the image.  Using
 *            temporary files means writing to disk which is slow.
 * Input:
 *            id:          thread id (offset into arrays at top of this file)
 *            image_type:  string of the image subtype e.g. "png", "jpeg"
 *            use_temp_file:
 *                         do we create a temporary file to store image data,
 *                         or can we use a pipe and avoid writing to disk
 * Output:
 *            None
 * Return:
 *            None
 * Exception:
 *            java.lang.InternalError if the image_type is unknown, or if
 *            a native error occurs.
 * Class:     vlc_net_content_image_ImageDecoder
 * Desc:
 * Method:    initDecoder
 * Signature: (ILjava/lang/String;Z)V
 */
JNIEXPORT void JNICALL
Java_vlc_net_content_image_ImageDecoder_initDecoder
(JNIEnv *env, jobject obj, jint id, jstring image_type, jboolean use_temp_file)
{
   int i = 0;
   int init_successful = JNI_FALSE;
   const char *str;
   char buf[100];
   char tmpname[L_tmpnam];
   Parameters params;
   int* fd;

   fd = fd_list[id];

   /* obtain a C representation of the java string */
   str = (*env)->GetStringUTFChars(env, image_type, 0);

   /* search for the given image type and if found, perform initialisation */
   do {
      if (STRSAME(str, available_types[i].type_string)) {
         param_list[id] = available_types[i].init_func();
         params = param_list[id];
         if (params != NULL) {
            init_successful = JNI_TRUE;
         }
         else {
            /* No memory?, hopefully we'll never see this */
            throw_exception(env, "java/lang/OutOfMemoryError", NULL);
         }
      }
      else {
         i++;
      }
   } while(!init_successful && (i<NUM_KNOWN_TYPES));

   /* ensure that the fptr is NULL */
   params->fptr = NULL;

   /* setup error message string */
   if (!init_successful)
      sprintf(buf, "Unknown file type: '%s'", str);

   /* release the java string now we are finished with it */
   (*env)->ReleaseStringUTFChars(env, image_type, str);

   /* throw exception */
   if (!init_successful)
      throw_exception(env, "java/lang/InternalError", buf);

   /* If using green threads under a *nix system, a blocking thread */
   /* will block the entire process.  For this reason we can not */
   /* safely use a pipe to transfer data incase the reading end of */
   /* the pipe blocks.  So we use a temporary file to store the */
   /* image data and read from that file */
   if (use_temp_file) {
      /* create a temporary file and open it for writing */
      for( ; ; ) {
         /* generate a name */
         tmpnam(tmpname);

         /* ensure we create this file sucessfully */
         if ( (fd[1] = creat(tmpname, S_IREAD | S_IWRITE)) >= 0)
            break;
      }

      /* now open the temporary file for writing */
      params->fptr = fopen(tmpname, "rb");

      /* now unlink the file.  this will remove the file from */
      /* the directory, but will not reclaim disk space until */
      /* all descriptors referencing the file are closed */
      unlink(tmpname);
   }
   else {
      /* prepare for receiving data */
      if (pipe(fd) == -1) {
         /* throw exception */
         throw_exception(env, "java/lang/InternalError",
                         "Could not create pipe");
      }

      /* wrap a file pointer around the pipe descriptor */
      params->fptr = fdopen(fd[0], "rb");
   }

   /* ensure it worked */
   if (params->fptr == NULL)
      throw_exception(env, "java/lang/InternalError", "fdopen error");
}

/*
 * Desc:      Sends data from the image file to the library.  This library
 *            receives the data to decode, not from a disk file, but rather
 *            as a stream of data.  This function provides the library with
 *            that stream of data.
 * Input:
 *            id:          thread id (offset into arrays at top of this file)
 *            data:        byte array containing raw data
 *            size:        number of elements in the array.  A value of -1
 *                         signals that there is no more data to be read.
 * Output:
 *            None
 * Return:
 *            None
 * Exception:
 *            None
 * Class:     vlc_net_content_image_ImageDecoder
 * Method:    sendData
 * Signature: (I[BI)V
 */
JNIEXPORT void JNICALL
Java_vlc_net_content_image_ImageDecoder_sendData
(JNIEnv *env, jobject obj, jint id, jbyteArray data, jint size)
{
   jbyte *ptr;
   int* fd;
   int err;

   fd = fd_list[id];

   if (size != -1) {
      /* grab data elements */
      ptr = (*env)->GetByteArrayElements(env, data, 0);

      /* a blocking write to the pipe */
      if ( (err = write(fd[1], (void *) ptr, (size_t) size)) == -1)
      {
         /* error occured, so close the pipe */
         if (errno != EBADF)
            close(fd[1]);
      }

      (*env)->ReleaseByteArrayElements(env, data, ptr, 0);
   }
   else {
      /* at end of data, so close pipe */
      close(fd[1]);
   }
}

/*
 * Desc:      Starts the decoding process.  This will result in the header
 *            of the image being read.  It is guaranteed that after this
 *            function returns, the image width and height will be available.
 * Input:
 *            id:          thread id (offset into arrays at top of this file)
 * Output:
 *            None
 * Return:
 *            None
 * Exception:
 *            java.lang.InternalError on error with the image decoding,
 *            or if the library has not yet been initialised.
 * Class:     vlc_net_content_image_ImageDecoder
 * Method:    startDecoding
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_vlc_net_content_image_ImageDecoder_startDecoding
(JNIEnv *env, jobject obj, jint id)
{
   Parameters params;

   if (!init) {
      throw_exception(env, "java/lang/InternalError",
                           "Library has not yet been initialised");
   }

   params = param_list[id];

   params->start_input(params);

   if (params->error)
      throw_exception(env, "java/lang/InternalError", params->error_msg);
}

/*
 * Desc:      Returns the image width.  This will return an undefined value
 *            before startDecoding() sucessfully completes.
 * Input:
 *            id:          thread id (offset into arrays at top of this file)
 * Output:
 *            None
 * Return:
 *            The width of the image
 * Exception:
 *            None
 * Class:     vlc_net_content_image_ImageDecoder
 * Method:    getImageWidth
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_vlc_net_content_image_ImageDecoder_getImageWidth
(JNIEnv *env, jobject obj, jint id)
{
   Parameters params;

   params = param_list[id];

   return (jint) params->width;
}

/*
 * Desc:      Returns the image height.  This will return an undefined value
 *            before startDecoding() sucessfully completes.
 * Input:
 *            id:          thread id (offset into arrays at top of this file)
 * Output:
 *            None
 * Return:
 *            The height of the image
 * Exception:
 *            None
 * Class:     vlc_net_content_image_ImageDecoder
 * Method:    getImageHeight
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_vlc_net_content_image_ImageDecoder_getImageHeight
(JNIEnv *env, jobject obj, jint id)
{
   Parameters params;

   params = param_list[id];

   return (jint) params->height;
}

/*
 * Desc:      Returns the number of color components of the image. This will return an
 *            undefined value before startDecoding() sucessfully completes.
 * Input:
 *            id:          thread id (offset into arrays at top of this file)
 * Output:
 *            None
 * Return:
 *            The width of the image
 * Exception:
 *            None
 * Class:     vlc_net_content_image_ImageDecoder
 * Method:    getImageWidth
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_vlc_net_content_image_ImageDecoder_getNumColorComponents
(JNIEnv *env, jobject obj, jint id)
{
   Parameters params;

   params = param_list[id];

   return (jint) params->numComponents;
}


/*
 * Desc:      Returns the next row of the image.  This function will keep
 *            track of which is the current row to return.
 * Input:
 *            id:          thread id (offset into arrays at top of this file)
 * Output:
 *            pixel_row:   array which will contain a rows worth of pixel
 *                         data.
 * Return:
 *            None
 * Exception:
 *            java.lang.InternalError on error with the image decoding
 * Class:     vlc_net_content_image_ImageDecoder
 * Method:    getNextImageRow
 * Signature: (I[I)V
 */
JNIEXPORT void JNICALL
Java_vlc_net_content_image_ImageDecoder_getNextImageRow
(JNIEnv *env, jobject obj, jint id, jintArray pixel_row)
{
   jint *ptr;
   Parameters params;

   params = param_list[id];

   ptr = (*env)->GetIntArrayElements(env, pixel_row, 0);

   /* we want to get data returned in this data */
   params->buffer = ptr;

   /* retrieve data */
   /* data will be returned as ints in ARGB format */
   params->get_pixel_row(params);
   params->row_num++;

   (*env)->ReleaseIntArrayElements(env, pixel_row, ptr, 0);

   if (params->error)
      throw_exception(env, "java/lang/InternalError", params->error_msg);
}

/*
 * Desc:      Performs any cleanup after the image has been decoded, including
 *            releasing resources.  This MUST always be called after an
 *            image has been decoded.
 * Input:
 *            id:          thread id (offset into arrays at top of this file)
 * Output:
 *            None
 * Return:
 *            None
 * Exception:
 *            None
 * Class:     vlc_net_content_image_ImageDecoder
 * Method:    finishDecoding
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_vlc_net_content_image_ImageDecoder_finishDecoding
(JNIEnv *env, jobject obj, jint id)
{
   Parameters params;

   params = param_list[id];

   if (params) {
      if (params->fptr)
         fclose(params->fptr);
      params->finish_input(params);
   }
}

