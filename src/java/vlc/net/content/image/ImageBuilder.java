/*****************************************************************************
 *                     The Virtual Light Company Copyright(c)1999
 *                                         Java Source
 *
 * This code is licensed under the GNU Library GPL. Please read license.txt
 * for the full details. A copy of the LGPL may be found at
 *
 * http://www.gnu.org/copyleft/lgpl.html
 *
 ****************************************************************************/

package vlc.net.content.image;

// Standard imports
import java.awt.Image;
import java.awt.Toolkit;
import java.awt.image.*;
import java.io.*;

/**
 * This is a generic decoder class that will load and create an image
 * from a given input stream.
 * <p>
 *
 * The actual decoding mechanism is located in native libraries, and this
 * class simply calls the necessary functions.  This code is optimised to
 * favour large images.
 * <P>
 *
 * This softare is released under the
 * <A HREF="http://www.gnu.org/copyleft/lgpl.html">GNU LGPL</A>
 *
 * @author  Justin Couch
 * @version $Revision: 1.1 $
 */
public class ImageBuilder
{
    /** Flag to say we are using JDK 1.1 */
    private static final boolean jdk1_1;

    /** Flag for the threading model */
    private static final boolean hasNativeThreads;

    /** Valid image types from the underlying native lib */
    private static String[] validTypes;

    /** the type of image we are decoding */
    private String imageType = null;

    /** used for synchronising while waiting for data to be sent */
    private Object finishLock = new Object();

    /** has all the data been sent to the native code yet? */
    private boolean sendingData;

    /**
     * Static initializer to set up the native library and find out what is
     * available to the system.
     */
    static
    {
        validTypes = ImageDecoder.getFileFormats();

        String ver = System.getProperty("java.version");
        String os_name = System.getProperty("os.name");
        String os_arch = System.getProperty("os.arch");

        jdk1_1 = ver.startsWith("1.1");

        hasNativeThreads = (os_name.startsWith("Win") &&
                            os_arch.startsWith("x86")) ||
                           !(ver.startsWith("1.1") || ver.startsWith("1.0"));
    }

    /**
     * Creates a new image decoder ready to decode data of the given image
     * type.
     *
     * @param the type of the image to decode.  This is the subtype of the
     *   mime image type, e.g. gif, jpeg, tiff, png.
     * @throws IllegalArgumentException if an invalid type is given
     */
    public ImageBuilder(String type)
    {
        if((type == null) || (type.length() == 0))
            throw new IllegalArgumentException("No image type given");

        // save type
        imageType = type;
        boolean valid = false;

        // ensure that the library can handle this image type
        for(int i = 0; i < validTypes.length; i++)
        {
            if(type.equalsIgnoreCase(validTypes[i]))
            {
                valid = true;
                break;
            }
        }

        if(!valid)
            throw new IllegalArgumentException("Unsupported image type " +
                                               type);
    }

    /**
     * Decodes the given image stream in the appropriate image type and return
     * it as an image.
     *
     * @param is input stream containing the image data in specified format.
     * @return the decoded image
     * @throws IOException on errors decoding the image.
     */
    public Image decode(InputStream is)
        throws IOException
    {
        int i;
        int[] data;

        ImageDecoder decoder = new ImageDecoder();

        // our id, in the range 0 to MAX_THREADS-1
        int thread_id = decoder.acquireThreadId();

        int width;
        int height;
        ImageBuffer imBuffer = null;
        BufferFiller filler = null;

        try
        {
            // perform initialisation
            decoder.initDecoder(thread_id, imageType, !hasNativeThreads);

            // start sending data to be decoded
            filler = new BufferFiller(thread_id, is, decoder, finishLock);

            // A problem exists with green threads(native code).
            // If a thread blocks, then the entire process blocks.
            // This is bad as it means that we cannot use a pipe
            // to transfer data to the native code, we must use a
            // temp file.
            // Under windoze, threading works so we don't need a
            // temp file.
            if(hasNativeThreads)
            {
                // start a new thread to feed data to the pipe on the native side
                Thread th = new Thread(filler);
                th.start();
            }
            else
            {
                // send all data to a temp file on the native side
                filler.run();
            }

            // start the decoding
            decoder.startDecoding(thread_id);

            // decoding has been started so we can now get image dimensions
            width = decoder.getImageWidth(thread_id);
            height = decoder.getImageHeight(thread_id);

            if(jdk1_1)
                imBuffer = new ImageBuffer(width, height);

            data = createIntArray(width*height);

            // temporary buffer to receive data one row at a time
            int[] tmpBuffer = new int[width];

            // now extract the image data
            for(i=0; i<height; i++)
            {
                decoder.getNextImageRow(thread_id, tmpBuffer);

                /* JDK1.1 - uncomment this, and comment out the line directly below
                imBuffer.setImageRow(i, tmpBuffer);
                */
                System.arraycopy(tmpBuffer, 0, data, i*width, width);
            }
        }
        catch(InternalError e1)
        {
            // error occured, so halt the BufferFiller if it is still going
            filler.die();

            // any errors, just pass them on
            throw new IOException(e1.getMessage());
        }
        catch(OutOfMemoryError e2)
        {
            // error occured, so halt the BufferFiller if it is still going
            filler.die();

            // any errors, just pass them on
            throw new IOException("Not enough memory");
        }
        finally
        {
            // Ensure that we perform cleanup
            decoder.finishDecoding(thread_id);

            // ensure that the buffer filler has completed before freeing
            // resources
            while(sendingData)
            {
                synchronized(finishLock)
                {
                    try
                    {
                        // wait to be notified that the buffer filler object
                        // has finished sending data
                        finishLock.wait();
                    }
                    catch(InterruptedException e3)
                    {
                    }
                }
            }


            // we have finished with the native library now
            decoder.releaseThreadId(thread_id);
        }

        Image ret_val = null;

        if(jdk1_1)
            ret_val = Toolkit.getDefaultToolkit().createImage(imBuffer);
        else
        {
            ColorModel cModel = ColorModel.getRGBdefault();
            DataBufferInt intBuf = new DataBufferInt(data,(width * height));
            SampleModel sModel = cModel.createCompatibleSampleModel(width, height);

            // create our raster
            WritableRaster raster = Raster.createWritableRaster(sModel, intBuf, null);

            // now create and return our buffered image
            ret_val = new BufferedImage(cModel, raster, false, null);
        }

        return ret_val;
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

        for(int i=0; i<NUM_TRIES; i++)
        {
            try
            {
                // return array if we allocate memory
                return new int[size];
            }
            catch(OutOfMemoryError e)
            {
                // get current runtime instance
                if(runtime == null)
                    runtime = Runtime.getRuntime();

                // first run any finalisations.
                runtime.runFinalization();

                long free_memory = runtime.freeMemory();
                long was_free;
                int count = 0;

                do
                {
                    count++;
                    was_free = free_memory;
                    runtime.gc();

                    free_memory = runtime.freeMemory();
                }
                while((free_memory > was_free)&&(count <= LOOP_CHECK));
            }
        }

        // if we are here then we have done some garbage collection
        // so try allocating memory again.
        return new int[size];
    }
}

