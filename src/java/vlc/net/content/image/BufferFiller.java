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
import java.io.InputStream;
import java.io.IOException;

// Application specific imports
// none

/**
 * Intermediary class that sends the data to be decoded to the native side.
 * <p>
 *
 * This functionality is threaded as the native side uses a pipe to avoid
 * the use of temporary files. It implements runnable rather than extending
 * thread to avoid overheads of thread creation if it is not needed. There are
 * times when this class will run as just a straight object, rather than one
 * which is threaded.
 * <p>
 *
 * This softare is released under the
 * <a href="http://www.gnu.org/copyleft/lgpl.html">GNU LGPL</a>
 *
 * @author  Justin Couch
 * @version $Revision: 1.1 $
 */
class BufferFiller implements Runnable
{
    /** Number of bytes in the read buffer */
    private static final int BUF_SIZE = 8192;

    /** The thread ID */
    private int id;

    /** Stream to read data from */
    private InputStream stream;

    /** a premature death */
    private boolean timeToDie;

    /** Lock object for those waiting on it to die */
    private Object finish;

    /** Decoder used for convertinh bytes from the stream */
    private ImageDecoder decoder;

    /** has all the data been sent to the native code yet? */
    private boolean sendingData;

    /**
     * Construct an instance of the buffer filler for a specific thread.
     *
     * @param threadId id number to pass to native side
     * @param is the input stream containing data to be decoded.
     * @param dec The decoder to use to process image info
     * @param finishLock A lock object for thread management on die commands
     */
    BufferFiller(int threadId,
                 InputStream is,
                 ImageDecoder dec,
                 Object finishLock)
    {
        id = threadId;
        stream = is;
        decoder = dec;
        finish = finishLock;

        sendingData = true;

        // assume a long and fruitful life
        timeToDie = false;
    }

    //----------------------------------------------------------
    // Methods required by Runnable
    //----------------------------------------------------------

    /**
     * Starts the thread going.  This copies the contents of the input
     * stream to the native code.
     */
    public void run()
    {
        int count = 0;
        byte[] buffer = new byte[BUF_SIZE];
        int num_read = 0;

        // copy contents of the input stream to native code
        do
        {
            try
            {
                num_read = stream.read(buffer, 0, BUF_SIZE);
            }
            catch(IOException e)
            {
                // error occured reading!
                System.err.println("error reading from input stream");
            }

            // send the data.  Note that this also sends a -1 when
            // the input stream is exhausted.  This is correct behaviour
            // as the -1 is used to signal the native side that input
            // has finished
            decoder.sendData(id, buffer, num_read);

            // yield every so often to allow other threads to run
            if(++count >= 5)
            {
                count = 0;
                Thread.currentThread().yield();
            }

        }
        while((num_read != -1) && (!timeToDie));

        // we have finished sending data, so inform the ImageDecoder
        synchronized(finish)
        {
            sendingData = false;
            finish.notifyAll();
        }
    }

    //----------------------------------------------------------
    // Local methods
    //----------------------------------------------------------

    /**
     * Informs the thread that it should stop as soon as possible, cleaning
     * up after itself as usual.
     */
    void die()
    {
      timeToDie = true;
    }

    /**
     * Check to see if it still sending data.
     *
     * @returns True if still sending data to the decoder
     */
    boolean stillSending()
    {
        return sendingData;
    }
}
