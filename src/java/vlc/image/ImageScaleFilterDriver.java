/*****************************************************************************
 *                     The Virtual Light Company Copyright(c) 2007
 *                                         Java Source
 *
 * This code is licensed under the GNU Library GPL. Please read license.txt
 * for the full details. A copy of the LGPL may be found at
 *
 * http://www.gnu.org/copyleft/lgpl.html
 *
 ****************************************************************************/

package vlc.image;

// External imports
import java.nio.ByteBuffer;

// Local imports
// none

/**
 * A generic scale filter class that handles the interface between Java
 * code and the native library which performs the actual scaling operation.
 *
 * @author Rex Melton
 * @version $Revision: 1.1 $
 */
public class ImageScaleFilterDriver {
	
	/** Maximum number of threads that can access the library at a time */
	private static final int MAX_THREADS = 10;
	
	/** array to record which threads are currently using the library */
	private static boolean[] threadInUse;
	
	/** mutex object for this class */
	private static Object mutex = new Object( );
	
	// load library and initialise it
	static {
		System.loadLibrary( "image_decode" );
		
		threadInUse = new boolean[MAX_THREADS];
		
		try {
			for ( int i = 0; i < MAX_THREADS; i++ )
				threadInUse[i] = false;
			
			initialize( MAX_THREADS );
		}
		catch( InternalError e ) {
			System.err.println( "Error initialising library: " + e.getMessage( ) ) ;
		}
	}
	
	/**
	 * Construct a new scale filter driver
	 */
	public ImageScaleFilterDriver( ) {
	}
	
	/**
	 * Request to become a user of an image scale filter. The driver supports
	 * a maximum number of simultaneous threads and won't let access happen until a
	 * thread ID is given. This is then used for all subsequent accesses until
	 * that thread is no longer needed. After that point, the thread ID should
	 * be released.
	 * <p>
	 * This is a blocking call that will not return until a thread ID has been
	 * made available.
	 *
	 * @return The ID to use in subsequent accesses
	 */
	public int acquireThreadId( ) {
		// the id, in the range 0 to MAX_THREADS-1
		int threadId = -1;
		
		// obtain a unique thread id
		// Note that this is a busy wait loop.
		// However, the busy wait will only occur when more than
		// MAX_THREADS threads are currently decoding an image
		//(which *should* be very unlikely)
		out:
			synchronized( mutex ) {
				for ( ; ; ) {
					for ( int i = 0; i < MAX_THREADS; i++ ) {
						if ( !threadInUse[i] ) {
							threadInUse[i] = true;
							threadId = i;
							break out;
						}
					}
					Thread.currentThread( ).yield( );
				}
			} 
		return( threadId );
	}
	
	/**
	 * Release the thread ID back to the global pool of available IDs.
	 *
	 * @param id The ID of the thread to release
	 */
	public void releaseThreadId( int id ) {
		if ( id < 0 || id >= MAX_THREADS ) {
			throw new IllegalArgumentException("Invalid thread ID");
		}
		
		// Catch all, should never hit this, for debugging purposes only.
		if( !threadInUse[id] ) {
			throw new IllegalArgumentException("ID was not in use");
		}
		
		threadInUse[id] = false;
	}
	
	//
	// Below are the function prototypes for the native methods that are
	// used to decode the image
	//
	
	/**
	 * Returns a list of scale filter types.
	 *
	 * @return array of strings containing descriptors of the scale filter types.
	 */
	public static native String[] getScaleFilterTypes( );
	
	/**
	 * Initialize the library to be able to handle the given number
	 * of threads.
	 *
	 * @param numThreads the number of threads to handle
	 * @exception InternalError if this is called more than once
	 */
	private static native void initialize( int numThreads )
		throws InternalError;
	
	/**
	 * Performs initialization prior to use.
	 * The filter type must be one of the valid types returned by
	 * getScaleFilterTypes().
	 *
	 * @param id Identify this thread to the native library
	 * @param filter_type Filter type. Must be one of the valid types returned by
	 * getScaleFilterTypes().
	 * @exception InternalError If filter_type is not recognised, or library has
	 * not been initialized.
	 * @see #getFileFormats
	 */
	native void initScaleFilter( int id, String filter_type )
		throws InternalError;
	
	/**
	 * Generate the scaled image data and place it in the argument
	 * destination byte buffer.
	 *
	 * @param id Identify this thread to the native library
	 * @param srcWidth The width of the source image
	 * @param srcheight The height of the source image
	 * @param srcCmp The number of components in the source image
	 * @param srcBuffer The source image data
	 * @param dstWidth The destination image width
	 * @param dstHeight The destination image height
	 * @param dstBuffer The buffer to initialize with the scaled image data
	 */
	native void scaleImage( int id, int srcWidth, int srcHeight, int srcCmp, ByteBuffer srcBuffer,
		int dstWidth, int dstHeight, ByteBuffer dstBuffer );
}

