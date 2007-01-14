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
import java.awt.color.ColorSpace;
import java.awt.Transparency;

// Application specific imports
// none

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
 * @version $Revision: 1.3 $
 */
public class ImageBuilder
{
    /**
     * Flag for requesting the decoded as a BufferedImage. If possible, this
     * will generate a BufferedImage.
     */
    public static final int IMAGE_REQD = 1;

    /** Flag for requesting the decoded as a ImageProducer */
    public static final int IMAGEPRODUCER_REQD = 2;

    /** Flag for requesting the decoded as a non-writable Raster */
    public static final int RASTER_REQD = 3;

    /** Flag for requesting the decoded image as a WritableRaster  */
    public static final int WRITABLE_RASTER_REQD = 4;


    /** Flag to say we are using JDK 1.1 */
    private static final boolean jdk1_1;

    /** Flag for the threading model */
    private static final boolean hasNativeThreads;

    /** Valid image types from the underlying native lib */
    private static final String[] validTypes;

    /** the type of image we are decoding */
    private String imageType;

    /** used for synchronising while waiting for data to be sent */
    private Object finishLock;

    /** has all the data been sent to the native code yet? */
    private boolean sendingData;

    /**
     * Static initializer to set up the native library and find out what is
     * available to the system.
     */
    static
    {
        validTypes = ImageDecoder.getFileFormats();

        String vm = System.getProperty("java.vm.info","");
        String ver = System.getProperty("java.version");

        jdk1_1 = ver.startsWith("1.1");

        hasNativeThreads = vm.indexOf("green") == -1;
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
        finishLock = new Object();
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
     * it as the object type requested. If this is JDK 1.1, ignore the request
     * if it is for a Raster object, and only return an Image.
     *
     * @param is input stream containing the image data in specified format.
     * @param type The requested image output type
     * @return the decoded image
     * @throws IOException on errors decoding the image.
     */
    public Object decode(InputStream is, int type)
        throws IOException
    {
        int i;
        int[] data;

        ImageDecoder decoder = new ImageDecoder();

        // our id, in the range 0 to MAX_THREADS-1
        int thread_id = decoder.acquireThreadId();

        int width;
        int height;
        int num_components;

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
            num_components = decoder.getNumColorComponents(thread_id);

            if(jdk1_1 || (type == IMAGEPRODUCER_REQD))
                imBuffer = new ImageBuffer(width, height, num_components);

            data = createIntArray(width*height);

            // temporary buffer to receive data one row at a time
            int[] tmpBuffer = new int[width];

            // now extract the image data
            if(jdk1_1 || (type == IMAGEPRODUCER_REQD))
            {
                for(i = 0; i < height; i++)
                {
                    decoder.getNextImageRow(thread_id, tmpBuffer);
                    imBuffer.setImageRow(i, tmpBuffer);
                }
            }
            else
            {
                for(i = 0; i < height; i++)
                {
                    decoder.getNextImageRow(thread_id, tmpBuffer);
                    System.arraycopy(tmpBuffer, 0, data, i*width, width);
                }
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

        Object ret_val = null;

        if(type == IMAGEPRODUCER_REQD)
        {
            ret_val = imBuffer;
        }
        else if(jdk1_1)
        {
            ret_val = Toolkit.getDefaultToolkit().createImage(imBuffer);
        }
        else
        {
            ColorModel cm = getColorModel(num_components);
            SampleModel sm = cm.createCompatibleSampleModel(width, height);
            DataBuffer buffer;

            if (num_components == 1)
            {
                int len = data.length;

                byte[] new_data = new byte[len];
                int idx = 0;
                for(int j=0; j < len; j++) {
                    new_data[idx++] = (byte) (data[j] & 0xFF);
                }

                buffer = new DataBufferByte(new_data, (width * height));
            } else
            {
                buffer = new DataBufferInt(data, (width * height));
            }

            switch(type)
            {
                case IMAGE_REQD:
                    // create our raster
                    WritableRaster raster =
                        Raster.createWritableRaster(sm, buffer, null);

                    // now create and return our buffered image
                    ret_val = new BufferedImage(cm, raster, false, null);
                    break;

                case RASTER_REQD:
                    ret_val = Raster.createRaster(sm, buffer, null);
                    break;

                case WRITABLE_RASTER_REQD:
                    ret_val = Raster.createWritableRaster(sm, buffer, null);
                    break;
            }

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

    /**
     * Create a colour model instance for the number of components
     *
     * @param numComponents The number of components in the image
     * @return A corresponding colour model for the information
     */
    private ColorModel getColorModel(int numComponents)
    {
        ColorModel ret_val = null;

        switch(numComponents) {
            case 1:
                ColorSpace cs = ColorSpace.getInstance(ColorSpace.CS_GRAY);

                int colorType = DataBuffer.TYPE_BYTE;
                int[] nBits = {8};
                ret_val = new ComponentColorModel(cs, nBits, false, false, Transparency.OPAQUE, colorType);

                break;

            case 2:
                ret_val = new DirectColorModel(ColorSpace.getInstance(ColorSpace.CS_sRGB),16, 0x00FF, 0x00FF, 0x00FF, 0xFF00, false, DataBuffer.TYPE_INT);
                break;

            case 3:
                ret_val = new DirectColorModel(24, 0xFF0000, 0xFF00, 0xFF, 0);

                break;

            case 4:
                ret_val =
                    new DirectColorModel(32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
                break;
        }

        return ret_val;
    }
}

