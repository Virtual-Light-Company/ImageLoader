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
import java.nio.ByteOrder;

// Local imports
import vlc.image.ByteBufferImage;

/**
 * The generic scale filter class that will scale images using the
 * requested native scale filter type.
 *
 * @author Rex Melton
 * @version $Revision: 1.1 $
 */
public class ImageScaleFilter {
	
	/** Valid filter types from the underlying native lib */
	private static final String[] validTypes;
	
	/** The filter type currently in use */
	private String filterType;
	
	/**
	 * Static initializer to set up the native library and determine which
	 * filter types are available.
	 */
	static {
		validTypes = ImageScaleFilterDriver.getScaleFilterTypes();
	}
	
	/**
	 * Construct a new image scale filter of the specified type
	 *
	 * @param type The scale filter type to use.
	 * @throws IllegalArgumentException if an invalid type is specified
	 */
	public ImageScaleFilter( String type ) {
		if( ( type == null ) || ( type.length( ) == 0 ) ) {
			throw new IllegalArgumentException( "Unsupported scale filter type " + type );
		}
		
		filterType = type;
		boolean valid = false;
		
		// ensure that the library can handle this filter type
		for( int i = 0; i < validTypes.length; i++ ) {
			if( type.equalsIgnoreCase( validTypes[i] ) ) {
				valid = true;
				break;
			}
		}
		
		if( !valid ) {
			throw new IllegalArgumentException( "Unsupported scale filter type " + type );
		}
	}
	
	/**
	 * Return an image scaled to the argument width and height parameters of
	 * the argument source image. The returned image will be of the same type
	 * (i.e. number of components) as the source imge.
	 *
	 * @param srcImage The source image to a create scaled image from
	 * @param dstWidth the width of the scaled image to return
	 * @param dstHeight the height of the scaled image to return
	 * @return The scaled image
	 */
	public ByteBufferImage getScaledImage( ByteBufferImage srcImage, int dstWidth, int dstHeight ) {
		
		ImageScaleFilterDriver driver = new ImageScaleFilterDriver( );
		
		// our id, in the range 0 to MAX_THREADS-1
		int thread_id = driver.acquireThreadId( );
		
		ByteBufferImage dstImage = null;
		
		try {
			// initialize
			driver.initScaleFilter( thread_id, filterType );
			
			int srcWidth = srcImage.getWidth( );
			int srcHeight = srcImage.getHeight( );
			int numCmp = srcImage.getType( );
			ByteBuffer srcBuffer = srcImage.getBuffer( );
			
			ByteBuffer dstBuffer = ByteBuffer.allocateDirect( dstWidth * dstHeight * numCmp );
			dstBuffer.order( ByteOrder.nativeOrder( ) );
			
			driver.scaleImage( thread_id, srcWidth, srcHeight, numCmp, srcBuffer,
				dstWidth, dstHeight, dstBuffer );
			
			dstImage = new ByteBufferImage( dstWidth, dstHeight, numCmp, srcImage.isGrayScale( ), dstBuffer );
		}
		catch( InternalError e1 ) {
			// any errors, just pass them on
			throw new IllegalArgumentException( e1.getMessage( ) );
		}
		finally {
			// we have finished with the native library now
			driver.releaseThreadId( thread_id );
		}
		return( dstImage );
	}
	
	/**
	 * Return the image data of an image scaled to the argument width and height
	 * parameters of the argument source image. The returned image will be of the 
	 * same type (i.e. number of components) as the source image.
	 *
	 * @param srcWidth the width of the source image
	 * @param srcHeight the height of the source image
	 * @param srcCmp the number of components in the source image
	 * @param srcBuffer the image data of the source image
	 * @param dstWidth the width of the scaled image to return
	 * @param dstHeight the height of the scaled image to return
	 * @return The image data of the scaled image
	 */
	public ByteBuffer getScaledImage( int srcWidth, int srcHeight, int numCmp, ByteBuffer srcBuffer, 
		int dstWidth, int dstHeight ) {
		
		ImageScaleFilterDriver driver = new ImageScaleFilterDriver( );
		
		// our id, in the range 0 to MAX_THREADS-1
		int thread_id = driver.acquireThreadId( );
		
		ByteBuffer dstBuffer = null;
		
		try {
			// initialize
			driver.initScaleFilter( thread_id, filterType );
			
			dstBuffer = ByteBuffer.allocateDirect( dstWidth * dstHeight * numCmp );
			dstBuffer.order( ByteOrder.nativeOrder( ) );
			
			driver.scaleImage( thread_id, srcWidth, srcHeight, numCmp, srcBuffer,
				dstWidth, dstHeight, dstBuffer );
		}
		catch(InternalError e1) {
			// any errors, just pass them on
			throw new IllegalArgumentException( e1.getMessage( ) );
		}
		finally {
			// we have finished with the native library now
			driver.releaseThreadId( thread_id );
		}
		return( dstBuffer );
	}
}

