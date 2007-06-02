/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class vlc_image_ImageScaleFilterDriver */

#ifndef _Included_vlc_image_ImageScaleFilterDriver
#define _Included_vlc_image_ImageScaleFilterDriver
#ifdef __cplusplus
extern "C" {
#endif
#undef vlc_image_ImageScaleFilterDriver_MAX_THREADS
#define vlc_image_ImageScaleFilterDriver_MAX_THREADS 10L
/*
 * Class:     vlc_image_ImageScaleFilterDriver
 * Method:    getScaleFilterTypes
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_vlc_image_ImageScaleFilterDriver_getScaleFilterTypes
  (JNIEnv *, jclass);

/*
 * Class:     vlc_image_ImageScaleFilterDriver
 * Method:    initialize
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_vlc_image_ImageScaleFilterDriver_initialize
  (JNIEnv *, jclass, jint);

/*
 * Class:     vlc_image_ImageScaleFilterDriver
 * Method:    initScaleFilter
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_vlc_image_ImageScaleFilterDriver_initScaleFilter
  (JNIEnv *, jobject, jint, jstring);

/*
 * Class:     vlc_image_ImageScaleFilterDriver
 * Method:    scaleImage
 * Signature: (IIIILjava/nio/ByteBuffer;IILjava/nio/ByteBuffer;)V
 */
JNIEXPORT void JNICALL Java_vlc_image_ImageScaleFilterDriver_scaleImage
  (JNIEnv *, jobject, jint, jint, jint, jint, jobject, jint, jint, jobject);

#ifdef __cplusplus
}
#endif
#endif