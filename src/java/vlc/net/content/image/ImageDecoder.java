/*****************************************************************************
 *                The Virtual Light Company Copyright (c) 1999
 *                               Java Source
 *
 * This code is licensed under the GNU Library GPL. Please read license.txt
 * for the full details. A copy of the LGPL may be found at
 *
 * http://www.gnu.org/copyleft/lgpl.html
 *
 * Project:    Image Content Handlers
 *
 * Version History
 * Date        TR/IWOR  Version  Programmer
 * ----------  -------  -------  ------------------------------------------
 *
 ****************************************************************************/

//
// Programmer Note!!
// This class requires jdk1.2 as it makes use of BufferedImage.
// To modify this class to that it will run under jdk1.1.x, search
// for "JDK1.1" and read the comments in the code.
//

package vlc.net.content.image;

// Standard imports
import java.awt.Image;
import java.awt.Toolkit;
import java.awt.image.*;
import java.io.*;

/**
 * This is a generic decoder class that will load different types of image
 * formats from a given input stream.  The actual decoding mechanism is
 * located in native libraries, and this class simple calls the necessary
 * functions.  This code is optimised to favour large images.
 * <P>
 *
 * This softare is released under the
 * <A HREF="http://www.gnu.org/copyleft/lgpl.html">GNU LGPL</A>
 * <P>
 *
 * @author     <A HREF="mailto:justin@vlc.com.au">Justin Couch</A>
 * @version    1.1.2 19th July 1999
 */
public class ImageDecoder
{
   /** Maximum number of threads that can access the library at a time */
   private static final int MAX_THREADS = 10;

   /** array to record which threads are currently using the library */
   private static boolean[] threadInUse;

   /** used for mutual exclusion for this class */
   private static Object mutex = new Object();

   /** the type of image we are decoding */
   private String imageType = null;

   /** used for synchronising while waiting for data to be sent */
   private Object finish = new Object();

   /** has all the data been sent to the native code yet? */
   private boolean sendingData;

   // load library and initialise it
   static {
      System.loadLibrary("image_decode");

      threadInUse = new boolean[MAX_THREADS];
      try {
         for(int i=0; i<MAX_THREADS; i++)
            threadInUse[i] = false;

         initialize(MAX_THREADS);
      }
      catch(InternalError e) {
         System.err.println("Error initialising library: "+e.getMessage());
      }
   }

   /**
    * Creates a new image decoder ready to decode data of the given image type.
    * @param the type of the image to decode.  This is the subtype of the
    * mime image type, e.g. gif, jpeg, tiff, png.
    * @exception IllegalArgumentException if an invalid type is given
    */
   public ImageDecoder(String type)
   {
      String[] validTypes = getFileFormats();
      boolean valid = false;

      // save type
      imageType = type;

      // ensure that the library can handle this image type
      for(int i=0; i<validTypes.length; i++)
      {
         if (type.equalsIgnoreCase(validTypes[i]))
         {
            valid = true;
            break;
         }
      }

      if (!valid)
         throw new IllegalArgumentException("ImageDecoder(): Invalid type - "
                                            + type);
   }

   /**
    * Decodes the given image stream in the appropriate image type and return
    * it as an image.
    * @param is input stream containing the image data in specified format.
    * @return the decoded image
    * @exception IOException on errors decoding the image.
    */
   public Image decode(InputStream is)
      throws IOException
   {
      int i;
      int[] data;

      if (imageType == null)
      {
         throw new IOException("ImageDecoder.decode(): null image type!");
      }

      // our id, in the range 0 to MAX_THREADS-1
      int threadId = -1;

      // obtain a unique thread id
      // Note that this is a busy wait loop.
      // However, the busy wait will only occur when more than
      // MAX_THREADS threads are currently decoding an image
      // (which *should* be very unlikely)
      out:
      synchronized(mutex) {
         for( ; ; ) {
            for(i=0; i<MAX_THREADS; i++) {
               if (!threadInUse[i]) {
                  threadInUse[i] = true;
                  threadId = i;
                  break out;
               }
            }
            Thread.currentThread().yield();
         }
      }

      /* JDK1.1 - uncomment this
      // this is where we will place all the decoded image data
      ImageBuffer imBuffer = null;
      */

      int width, height;
      BufferFiller bf = null;

      try
      {
         boolean runningWindoze =
            ( System.getProperty("os.name").startsWith("Win") &&
              System.getProperty("os.arch").equals("x86") );

         // A problem exists with green threads (native code).
         // If a thread blocks, then the entire process blocks.
         // This is bad as it means that we cannot use a pipe
         // to transfer data to the native code, we must use a
         // temp file.
         // Under windoze, threading works so we don't need a
         // temp file.
         boolean useTempFile = !runningWindoze;

         // perform initialisation
         initDecoder(threadId, imageType, useTempFile);

         // start sending data to be decoded
         bf = new BufferFiller(threadId, is);

         // set the flag for sending data.
         // This will be unset when the BufferFiller object has sent all
         // the data
         sendingData = true;

         if (useTempFile) {
            // send all data to a temp file on the native side
            bf.run();
         }
         else {
            // start a new thread to feed data to the pipe on the native side
            bf.start();
         }

         // start the decoding
         startDecoding(threadId);

         // decoding has been started so we can now get image dimensions
         width = getImageWidth(threadId);
         height = getImageHeight(threadId);

         /* JDK1.1 - uncomment this, and comment out the line directly below
         // create a buffer large enough to hold all the data
         imBuffer = new ImageBuffer(width, height);
         */
         data = createIntArray(width*height);

         // temporary buffer to receive data one row at a time
         int[] tmpBuffer = new int[width];

         // now extract the image data
         for(i=0; i<height; i++)
         {
            getNextImageRow(threadId, tmpBuffer);

            /* JDK1.1 - uncomment this, and comment out the line directly below
            imBuffer.setImageRow(i, tmpBuffer);
            */
            System.arraycopy(tmpBuffer, 0, data, i*width, width);
         }
      }
      catch(InternalError e1)
      {
         // error occured, so halt the BufferFiller if it is still going
         bf.die();

         // any errors, just pass them on
         throw new IOException(e1.getMessage());
      }
      catch(OutOfMemoryError e2)
      {
         // error occured, so halt the BufferFiller if it is still going
         bf.die();

         // any errors, just pass them on
         throw new IOException("Not enough memory");
      }
      finally
      {
         // Ensure that we perform cleanup
         finishDecoding(threadId);

         // ensure that the buffer filler has completed before freeing
         // resources
         while(sendingData) {
            synchronized(finish) {
               try {
                  // wait to be notified that the buffer filler object
                  // has finished sending data
                  finish.wait();
               }
               catch(InterruptedException e3) {}
            }
         }


         // we have finished with the native library now
         threadInUse[threadId] = false;
      }

      /* JDK1.1 - uncomment this
      // now create an image from the buffered data
      return Toolkit.getDefaultToolkit().createImage(imBuffer);
        /*
      */

      ColorModel cModel = ColorModel.getRGBdefault();
      DataBufferInt intBuf = new DataBufferInt(data, (width * height));
      SampleModel sModel = cModel.createCompatibleSampleModel(width, height);

      // create our raster
      WritableRaster raster = Raster.createWritableRaster(sModel, intBuf, null);

      // now create and return our buffered image
      return new BufferedImage(cModel, raster, false, null);

      // JDK1.1 - uncomment this
      // */
      //
   }


   /**
    * Allocates an integer array of the given size.
    * If the initial attempt at creating the array fails, then the
    * garbage collector is run and the memory allocation is retried.
    * @param size number of integer elements in the array
    * @return the allocated array
    * @throws OutOfMemoryError if not enough memory
    */
   private int[] createIntArray(int size)
   {
      // just to prevent an infinite loop.  see below
      final int LOOP_CHECK = 20;

      // number of times to attempt memory allocation
      final int NUM_TRIES = 10;

      Runtime runtime = null;

      for(int i=0; i<NUM_TRIES; i++) {
         try {
            // return array if we allocate memory
            return new int[size];
         }
         catch(OutOfMemoryError e)
         {
            // get current runtime instance
            if (runtime == null)
               runtime = Runtime.getRuntime();

            // first run any finalisations.
            runtime.runFinalization();

            long free_memory = runtime.freeMemory();
            long was_free;
            int count = 0;

            do {
               count++;
               was_free = free_memory;
               runtime.gc();

               free_memory = runtime.freeMemory();
            } while((free_memory > was_free) && (count <= LOOP_CHECK));
         }
      }

      // if we are here then we have done some garbage collection
      // so try allocating memory again.
      return new int[size];
   }

   //
   // Below are the function prototypes for the native methods that are
   // used to decode the image
   //

   /**
    * Initialises the library to be able to handle the given number
    * of threads.
    * @param numThreads the number of threads to handle
    * @exception InternalError if this is called more than once
    */
   private static native void initialize(int numThreads)
      throws InternalError;

   /**
    * Returns a list of image file formats that this decoder can decode.
    * The list is an array of strings of the mime image subtype of the
    * image format.
    * @return array of strings.
    */
   public static native String[] getFileFormats();

   /**
    * Performs initialisation prior to decoding.
    * The image type is one of the valid types returned by getFileFormats().
    * @param id identify this thread to the native library
    * @param type image type must be one of the valid subtypes returned by
    * getFileFormats().
    * @param useTemp should the library use a temporary file to store the
    * image data, or can it use a pipe to avoid writing to disk
    * @exception InternalError if type is not recognised, or library has
    * not been initialised.
    * @see #getFileFormats
    */
   private native void initDecoder(int id, String type, boolean useTemp)
      throws InternalError;

   /**
    * Sends the data to be decoded to the native library.  Data is
    * sent in chunks, as a stream.
    * <P>
    * Note: this method should be private, but due to a java bug it must
    * be protected to allow the inner class 'BufferFiller' to access
    * this method.
    * @param id identify this thread to the native library
    * @param buffer array containing data
    * @param size number of valid bytes in the array, or -1 when signalling
    * that there is no more data
    */
   native void sendData(int id, byte[] buffer, int size);

   /**
    * Starts decoding the image.
    * @param id identify this thread to the native library
    * @exception InternalError if this is called before initialisation
    * has occured.
    */
   private native void startDecoding(int id)
      throws InternalError;

   /**
    * Returns the width of the image that is to be decoded.
    * @param id identify this thread to the native library
    * @return width of the image
    * @exception InternalError on unexpected error.
    */
   private native int getImageWidth(int id)
      throws InternalError;

   /**
    * Returns the height of the image that is to be decoded.
    * @param id identify this thread to the native library
    * @return height of the image
    * @exception InternalError on unexpected error.
    */
   private native int getImageHeight(int id)
      throws InternalError;

   /**
    * Returns the next decoded row of the image.
    * @param id identify this thread to the native library
    * @param array of integers representing pixel data for a row of the image,
    * to be filled in by this method.
    * @exception InternalError when trying to read more rows than exist
    * in the image file
    */
   private native void getNextImageRow(int id, int[] buffer)
      throws InternalError;

   /**
    * Performs any necessary cleanup on the native side when the image has
    * been fully decoded.
    * @param id identify this thread to the native library
    */
   private native void finishDecoding(int id);

   /**
    * This class sends the data to be decoded to the native side.  This
    * functionality is threaded as the native side uses a pipe to avoid
    * the use of temporary files.
    */
   private class BufferFiller extends Thread
   {
      private int id;
      private InputStream stream;

      /** a premature death */
      private boolean timeToDie;

      /**
       * Constructor.
       * @param threadId id number to pass to native side
       * @param is the input stream containing data to be decoded.
       */
      BufferFiller(int threadId, InputStream is)
      {
         // save thread id
         id = threadId;

         // save stream
         stream = is;

         // assume a long and fruitful life
         timeToDie = false;
      }

      /**
       * Informs the thread that it should stop as soon as possible, cleaning
       * up after itself as usual.
       */
      public void die()
      {
        timeToDie = true;
      }

      /**
       * Starts the thread going.  This copies the contents of the input
       * stream to the native code.
       */
      public void run()
      {
         int count = 0;
         final int BUF_SIZE = 8192;
         byte[] buffer = new byte[BUF_SIZE];
         int num_read = 0;

         // copy contents of the input stream to native code
         do {
            try {
               num_read = stream.read(buffer, 0, BUF_SIZE);
            }
            catch(IOException e) {
               // error occured reading!
               System.err.println("error reading from input stream");
            }

            // send the data.  Note that this also sends a -1 when
            // the input stream is exhausted.  This is correct behaviour
            // as the -1 is used to signal the native side that input
            // has finished
            sendData(id, buffer, num_read);

            // yield every so often to allow other threads to run
            if (++count >= 5) {
               count = 0;
               Thread.currentThread().yield();
            }

         } while( (num_read != -1) && (!timeToDie) );

         // we have finished sending data, so inform the ImageDecoder
         synchronized(finish) {
            sendingData = false;
            finish.notifyAll();
         }
      } // end run()
   }
}

