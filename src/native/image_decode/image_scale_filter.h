#ifndef _IMAGE_SCALE_FILTER_H
#define _IMAGE_SCALE_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct param* FilterParam;
	
	/* Structure to hold available scale filter types */
	typedef struct {
		FilterParam (*init_func)();   /* Initialization function for filter */
		char *type_string;             /* Filter type identifier */
	} FilterType;
	
	/* External reference to filter initialization function */
	extern FilterParam area_avg_init( );
	
	/* Number of known image scale filters */
	#define NUM_SCALE_FILTERS 1
	
	/* References to known filters */
	static FilterType available_scale_filter[] = {
		{area_avg_init, "AreaAverage"},     /* {initialization function, filter type identifier} */
	};
	
	#include <jni.h>
	#include "vlc_image_ImageScaleFilterDriver.h"
	
	/* Structure containing pointers to image data structures and pointer to scale function */
	struct param {
		int srcWidth;                           /* width of the image */
		int srcHeight;                          /* height of the image */
		int srcComponents;                      /* num components 1 - 4 */
		jbyte *src_pixel_data;                  /* pixels */
		int dstWidth;                           /* width of the image */
		int dstHeight;                          /* height of the image */
		int dstComponents;                      /* num components 1 - 4 */
		jbyte *dst_pixel_data;                  /* pixels */
		void (*scale_func)(FilterParam);       	/* function to perform the scale operation */
	};
	
#ifdef __cplusplus
}
#endif

#endif /* _IMAGE_SCALE_FILTER_H */
