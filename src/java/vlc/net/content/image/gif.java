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
import java.awt.*;
import java.awt.image.*;
import java.net.*;
import java.io.*;

// Application specific imports
//none

/**
 * Content handler to load Graphic Interchange Format files.
 * Mime type: image/gif
 * <P>
 *
 * This softare is released under the
 * <A HREF="http://www.gnu.org/copyleft/lgpl.html">GNU LGPL</A>
 * <P>
 *
 * @author     <A HREF="mailto:justin@vlc.com.au">Justin Couch</A>
 * @version    1.0.3 10th August 1999
 */
public class gif extends ContentHandler
{
   /**
    * Only set this to true if you have a LZW patent license from Unisys.
    * If this is false, then java's gif loader is used, otherwise
    * our custom one is
    */
   protected static final boolean HAVE_LZW_PATENT_LICENSE = false;

   /** Do we absolutely require a buffered image? */
   protected static final boolean WANT_BUFFERED_IMAGE = true;

   /**
    * Should we run the garbage collector before each call to decode an
    * image?  Running the garbage collector takes about 100ms, so is really
    * noticable with small images.  Unfortunately, not running the garbage
    * collector when loading large images can cause
    * <code>OutOfMemoryError</code>s to occur.  Catching the error, running
    * the garbage collector and then trying again doesn't work, because
    * the garbage collector is not aggressive enough and doesn't reclaim all
    * memory on each run.  If a certain vendor used reference counting instead
    * of mark and sweep, this problem wouldn't exist, or would be greatly
    * minimised.
    */
   protected static final boolean RUN_GC = false;

   /** the size of the buffer to use when reading data from a stream */
   private static final int BUF_SIZE = 8192;

   /**
    * Given a URL connect stream positioned at the beginning of the GIF
    * image file, this method reads that stream and creates an Image from it.
    *
    * @param u an URL connection.
    * @return the Image, or null on error.
    * @exception IOException if an I/O error occurs while reading the object.
    */
   public Object getContent(URLConnection u)
      throws IOException
   {
      if (HAVE_LZW_PATENT_LICENSE) {
         // create a new image decoder ready to decode a GIF image
         // NOT IMPLEMENTED YET
         ImageDecoder decoder = new ImageDecoder("gif");

         // now decode the image from the input stream
         return decoder.decode(u.getInputStream());
      }
      else {
         /* JDK1.1 - uncomment this
         // use java to get our image
         return Toolkit.getDefaultToolkit().getImage(u.getURL());
           /*
         */

         if (RUN_GC) {
            // first we have to run the garbage collector, to ensure that
            // we have maximum amount of memory available

            // get current runtime instance
            Runtime runtime = Runtime.getRuntime();

            // first run any finalisations.
            runtime.runFinalization();

            long free_memory = runtime.freeMemory();
            long was_free;
            int count = 0;

            // to prevent an infinite loop
            final int LOOP_CHECK = 5;

            do {
               count++;
               was_free = free_memory;
               runtime.gc();

               free_memory = runtime.freeMemory();
            } while((free_memory > was_free) && (count <= LOOP_CHECK));
         }

         Image img;

         try {
            // extract the data as a byte array
            byte[] byteArray = getImageAsBytes(u.getInputStream());

            // now create the image
            img = Toolkit.getDefaultToolkit().createImage(byteArray);
         }
         catch(OutOfMemoryError e0) {
            // turn error into exception to be consistant
            throw new IOException("Out of memory");
         }

         // return now if we don't need a buffered image
         if (!WANT_BUFFERED_IMAGE)
            return img;

         MediaTracker tracker = new MediaTracker(new Panel());
         try {
            tracker.addImage(img, 0);
            tracker.waitForID(0);
         }
         catch (InterruptedException e1) {
            handleErrorMessage("INTERRUPTED while loading Image");
            return null;
         }

         if (tracker.isErrorAny()) {
            handleErrorMessage(new String("Error loading image " +
                                           u.getURL().toString()));
            return null;
         }

         tracker.removeImage(img);
         tracker = null;

         int width = img.getWidth(null);
         int height = img.getHeight(null);

         BufferedImage bufImg = new BufferedImage(width, height,
                                                  BufferedImage.TYPE_INT_ARGB);
         Graphics g = bufImg.getGraphics();
         g.drawImage(img, 0, 0, null);

         g.dispose();
         img.flush();

         return bufImg;

         // JDK1.1 - uncomment this
         // */
         //
      }
   }

   /**
    * Sucks the given input stream and returns the contents as a byte array.
    * @param stream the input stream
    * @return the array of bytes found in the input stream
    */
   private byte[] getImageAsBytes(InputStream stream)
   {
      ByteArrayOutputStream os = new ByteArrayOutputStream();
      byte[] buf = new byte[BUF_SIZE];

      int numRead = -1;

      // now read all that the stream has, and store in in the
      // byte array output stream
      do {
         try {
            numRead = stream.read(buf, 0, BUF_SIZE);
         }
         catch(IOException e0) {
            // error occured reading!
            System.err.println("error reading from input stream");
         }

         if (numRead != -1)
            os.write(buf, 0, numRead);
      } while(numRead != -1);

      return os.toByteArray();
   }

   /**
    * Method provided to derived classes to override if they wish to
    * handle possible error messages differently to the default.
    * Default is to print message to stderr.
    * @param str The error message
    */
   protected void handleErrorMessage(String str)
   {
      // just print error message to stderr
      System.err.println(str);
   }
}

