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
 * Common routines shared by many image decoders.
 */

#include "decode_image.h"

/*
 * Allocates a 2D array of bytes (char).
 */
U_CHAR **alloc2DByteArray(int rows, int cols)
{
   U_CHAR **arr;
   int row, i;
   int fail;

   /* Allocate space for row pointers +1 for NULL terminator */
   arr = (U_CHAR **) malloc ((rows+1) * sizeof(U_CHAR *));
   arr[rows] = NULL;

   /* Allocate the individual rows */
   if (arr) {
      fail = JNI_FALSE;
      row = 0;
      while((!fail && (row < rows))) {
         arr[row] = (U_CHAR *) malloc(cols);

         /* check to make sure memory allocation suceeded */
         if (arr[row] == NULL) {
            for(i=0; i<row; i++)
               free(arr[i]);
            free(arr);

            fail = JNI_TRUE;
         }
         else
            row++;
      }
   }
   else
      fail = JNI_TRUE;

   if (fail)
      arr = NULL;

   return arr;
}

/*
 * Frees a 2D Byte array allocated by alloc2DByteArray
 */
void free2DByteArray(U_CHAR **arr)
{
   U_CHAR **ptr = arr;

   /* free all of the rows, plus the array of pointers */
   if (arr != NULL) {
      while(*ptr != NULL)
         free(*ptr++);

      free(arr);
   }
}

/*
 * Allocates a 2D array of Java Ints (jint).
 */
jint **alloc2DJIntArray(int rows, int cols)
{
   jint **arr;
   int row, i;
   int fail;

   /* Allocate space for row pointers +1 for NULL terminator */
   arr = (jint **) malloc ((rows+1) * sizeof(jint *));
   arr[rows] = NULL;

   /* Allocate the individual rows */
   if (arr) {
      fail = JNI_FALSE;
      row = 0;
      while((!fail && (row < rows))) {
         arr[row] = (jint *) malloc(cols * sizeof(jint));

         /* check to make sure memory allocation suceeded */
         if (arr[row] == NULL) {
            for(i=0; i<row; i++)
               free(arr[i]);
            free(arr);

            fail = JNI_TRUE;
         }
         else
            row++;
      }
   }
   else
      fail = JNI_TRUE;

   if (fail)
      arr = NULL;

   return arr;
}

/*
 * Frees a 2D Java Int array allocated by alloc2DJIntArray
 */
void free2DJIntArray(jint **arr)
{
   jint **ptr = arr;

   /* free all of the rows, plus the array of pointers */
   if (arr != NULL) {
      while(*ptr != NULL)
         free(*ptr++);

      free(arr);
   }
}

