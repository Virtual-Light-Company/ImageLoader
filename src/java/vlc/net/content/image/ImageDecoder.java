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
// none

// Application specific imports
// none

/**
 * This is a generic decoder class that handles the interface between Java
 * code and the native library.
 * <p>
 *
 * Generally speaking you should not need to directly interface with this
 * class from user-land code. Normally you will use the builder or producer
 * classes, which then make calls to this class. This code is optimised to
 * favour large images.
 * <P>
 *
 * This softare is released under the
 * <A HREF="http://www.gnu.org/copyleft/lgpl.html">GNU LGPL</A>
 * <P>
 *
 * @author  Justin Couch
 * @version $Revision: 1.2 $
 */
public class ImageDecoder
{
    /** Maximum number of threads that can access the library at a time */
    private static final int MAX_THREADS = 10;

    /** array to record which threads are currently using the library */
    private static boolean[] threadInUse;

    /** used for mutual exclusion for this class */
    private static Object mutex = new Object();

    // load library and initialise it
    static
    {
        System.loadLibrary("image_decode");

        threadInUse = new boolean[MAX_THREADS];

        try
        {
            for(int i = 0; i < MAX_THREADS; i++)
                threadInUse[i] = false;

            initialize(MAX_THREADS);
        }
        catch(InternalError e)
        {
            System.err.println("Error initialising library: "+e.getMessage());
        }
    }

    /**
     * Creates a new image decoder ready to decode data of the given image type.
     * @param the type of the image to decode.  This is the subtype of the
     * mime image type, e.g. gif, jpeg, tiff, png.
     * @exception IllegalArgumentException if an invalid type is given
     */
    public ImageDecoder()
    {
    }

    /**
     * Request to become a user of the decoder. The decoder supports a maximum
     * number of simultaneous threads and won't let access happen until a
     * thread ID is given. This is then used for all subsequent accesses until
     * that thread is no longer needed. After that point, the thread ID should
     * be released.
     * <p>
     * This is a blocking call that will not return until a thread ID has been
     * made available.
     *
     * @return The ID to use in subsequent accesses
     */
    public int acquireThreadId()
    {
        // the id, in the range 0 to MAX_THREADS-1
        int threadId = -1;

        // obtain a unique thread id
        // Note that this is a busy wait loop.
        // However, the busy wait will only occur when more than
        // MAX_THREADS threads are currently decoding an image
        //(which *should* be very unlikely)
        out:
        synchronized(mutex)
        {
            for( ; ; )
            {
                for(int i = 0; i < MAX_THREADS; i++)
                {
                    if(!threadInUse[i])
                    {
                        threadInUse[i] = true;
                        threadId = i;
                        break out;
                    }
                }
                Thread.currentThread().yield();
            }
        }

        return threadId;
    }

    /**
     * Release the thread ID back to the global pool of available IDs.
     *
     * @param id The ID of the thread to release
     */
    public void releaseThreadId(int id)
    {
        if(id < 0 || id >= MAX_THREADS)
            throw new IllegalArgumentException("Invalid thread ID");

        // Catch all, should never hit this, for debugging purposes only.
        if(!threadInUse[id])
            throw new IllegalArgumentException("ID was not in use");

        threadInUse[id] = false;
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
     *
     * @param id identify this thread to the native library
     * @param type image type must be one of the valid subtypes returned by
     * getFileFormats().
     * @param useTemp should the library use a temporary file to store the
     * image data, or can it use a pipe to avoid writing to disk
     * @exception InternalError if type is not recognised, or library has
     * not been initialised.
     * @see #getFileFormats
     */
    native void initDecoder(int id, String type, boolean useTemp)
        throws InternalError;

    /**
     * Sends the data to be decoded to the native library.  Data is
     * sent in chunks, as a stream.
     * <P>
     * Note: this method should be private, but due to a java bug it must
     * be protected to allow the inner class 'BufferFiller' to access
     * this method.
     *
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
    native void startDecoding(int id)
        throws InternalError;

    /**
     * Returns the width of the image that is to be decoded.
     * @param id identify this thread to the native library
     * @return width of the image
     * @exception InternalError on unexpected error.
     */
    native int getImageWidth(int id)
        throws InternalError;

    /**
     * Returns the height of the image that is to be decoded.
     * @param id identify this thread to the native library
     * @return height of the image
     * @exception InternalError on unexpected error.
     */
    native int getImageHeight(int id)
        throws InternalError;

    /**
     * Returns the next decoded row of the image.
     * @param id identify this thread to the native library
     * @param array of integers representing pixel data for a row of the image,
     * to be filled in by this method.
     * @exception InternalError when trying to read more rows than exist
     * in the image file
     */
    native void getNextImageRow(int id, int[] buffer)
        throws InternalError;

    /**
     * Performs any necessary cleanup on the native side when the image has
     * been fully decoded.
     * @param id identify this thread to the native library
     */
    native void finishDecoding(int id);
}

