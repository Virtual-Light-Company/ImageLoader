
#include "image_scale_filter.h"

/* Flag indicating whether this library been initialized */
static int init = JNI_FALSE;

/* This is necessary for keeping track of the current state in between */
/* function calls */
static FilterParam *param_list;

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
	if (newExcCls == 0)
	{
		/* Unable to find the new exception class, give up. */
		return;
	}
	
	if (message == NULL)
	(*env)->ThrowNew(env, newExcCls, "");
	else
	(*env)->ThrowNew(env, newExcCls, message);
}

/*
 * Desc:      Returns an array of strings containing the supported filter types
 * Input:
 *            None
 * Output:
 *            None
 * Return:
 *            Array of strings containing supported filter types
 * Exception:
 *            None
 *
 * Class:     vlc_image_ImageScaleFilterDriver
 * Method:    getScaleFilterTypes
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL 
Java_vlc_image_ImageScaleFilterDriver_getScaleFilterTypes
(JNIEnv *env, jclass obj) {
	int i;
	jarray string_array;
	
	/* create an array of strings and initialize it to contain NULL objects */
	string_array = (*env)->NewObjectArray(env, 
	NUM_SCALE_FILTERS,
	(*env)->FindClass(env, "java/lang/String"),
	NULL);
	
	/* now fill the array with string objects representing the allowed */
	/* filter types */
	for(i=0; i<NUM_SCALE_FILTERS; i++) {
		(*env)->SetObjectArrayElement(env, string_array, i,
		(*env)->NewStringUTF(env, available_scale_filter[i].type_string));
	}
	
	return( string_array );
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
 *
 * Class:     vlc_image_ImageScaleFilterDriver
 * Method:    initialize
 * Signature: (I)V
 */
JNIEXPORT void JNICALL 
Java_vlc_image_ImageScaleFilterDriver_initialize
(JNIEnv *env, jclass obj, jint num_threads) {
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
	param_list = (FilterParam *) calloc(num_threads, sizeof(FilterParam));
	if (!param_list) {
		/* No memory?, hopefully we'll never see this */
		throw_exception(env, "java/lang/OutOfMemoryError", NULL);
	}
}

/*
 * Desc:      Initialize the scale filter library to process an image. This 
 *            function sets the type of scale filter that the library is to use.
 * Input:
 *            id:           thread id (offset into arrays at top of this file)
 *            filter_type:  string of the filter type
 * Output:
 *            None
 * Return:
 *            None
 * Exception:
 *            java.lang.InternalError if the filter_type is unknown, or if
 *            a native error occurs.
 *
 * Class:     vlc_image_ImageScaleFilterDriver
 * Method:    initScaleFilter
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_vlc_image_ImageScaleFilterDriver_initScaleFilter
(JNIEnv *env, jobject obj, jint id, jstring filter_type) {
	int i = 0;
	int init_successful = JNI_FALSE;
	const char *str;
	char buf[100];
	char tmpname[L_tmpnam];
	FilterParam params;
	
	/* obtain a C representation of the java string */
	str = (*env)->GetStringUTFChars(env, filter_type, 0);
	
	/* search for the given image type and if found, perform initialisation */
	do {
		if ( strcmp(str, available_scale_filter[i].type_string) == 0 ) {
			/* If there is something at this ID spot already, throw it away
			* and start again. */
			if(param_list[id]) {
				free(param_list[id]);
			}
			
			param_list[id] = available_scale_filter[i].init_func( );
			params = param_list[id];
			if (params != NULL) {
				init_successful = JNI_TRUE;
			} else {
				/* No memory?, hopefully we'll never see this */
				throw_exception(env, "java/lang/OutOfMemoryError", NULL);
			}
		} else {
			i++;
		}
	} while(!init_successful && (i<NUM_SCALE_FILTERS));
	
	/* setup error message string */
	if (!init_successful) {
		sprintf(buf, "Unknown filter type: '%s'", str);
	}
	
	/* release the java string now we are finished with it */
	(*env)->ReleaseStringUTFChars(env, filter_type, str);
	
	/* throw exception */
	if (!init_successful) {
		throw_exception(env, "java/lang/InternalError", buf);
	}
}

/*
 * Desc:      Initialize the destination byte buffer with the scaled image data.
 * Input:
 *            id:          thread id (offset into arrays at top of this file)
 *            srcWidth:    the source image width
 *            srcheight:   the source image height
 *            srcCmp:      the number of components in the source image
 *            srcBuffer:   the source image data 
 *            dstWidth:    the destination image width
 *            dstHeight:   the destination image height
 *            dstBuffer:   the buffer to initialize with the destination image data
 * Output:
 *            dstBuffer:   initialized with the scaled image data
 * Return:
 *            None
 * Exception:
 *            None
 *
 * Class:     vlc_image_ImageScaleFilterDriver
 * Method:    scaleImage
 * Signature: (IIIILjava/nio/ByteBuffer;IILjava/nio/ByteBuffer;)V
 */
JNIEXPORT void JNICALL 
Java_vlc_image_ImageScaleFilterDriver_scaleImage
(JNIEnv *env, jobject obj, jint id, jint srcWidth, jint srcHeight, jint srcCmp, jobject srcBuffer, 
	jint dstWidth, jint dstHeight, jobject dstBuffer) {
		
	FilterParam params;
	jbyte *srcPtr;
	jbyte *dstPtr;
	
	params = param_list[id];
	
	params->srcWidth = srcWidth;
	params->srcHeight = srcHeight;
	params->srcComponents = srcCmp;
		
	srcPtr = (*env)->GetDirectBufferAddress(env, srcBuffer);
	params->src_pixel_data = srcPtr;
	
	params->dstWidth = dstWidth;
	params->dstHeight = dstHeight;
	params->dstComponents = srcCmp;
	
	dstPtr = (*env)->GetDirectBufferAddress(env, dstBuffer);
	params->dst_pixel_data = dstPtr;
	
	params->scale_func(params);
		
	params->src_pixel_data = NULL;
	params->dst_pixel_data = NULL;
}

