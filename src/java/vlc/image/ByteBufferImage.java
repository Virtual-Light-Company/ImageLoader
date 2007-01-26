/*****************************************************************************
 *                The Virtual Light Company Copyright (c) 2007
 *                               Java Source
 *
 * This code is licensed under the GNU Library GPL. Please read license.txt
 * for the full details. A copy of the LGPL may be found at
 *
 * http://www.gnu.org/copyleft/lgpl.html
 *
 ****************************************************************************/

package vlc.image;

// External imports
import java.nio.Buffer;
import java.nio.ByteBuffer;

// Local imports
// none

/**
 * A representation of an image contained in a <code>ByteBuffer</code>.
 *
 * @author Rex Melton
 * @version $Revision: 1.1 $
 */
public class ByteBufferImage { 
	
	/** The INTENSITY type, a 1 component image */
	public static final int INTENSITY = 1;
	
	/** The INTENSITY_ALPHA type, a 2 component image */
	public static final int INTENSITY_ALPHA = 2;
	
	/** The RGB type, a 3 component image */
	public static final int RGB = 3;
	
	/** The RGBA type, a 4 component image */
	public static final int RGBA = 4;
	
	/** Invalid width error message */
	private static final String INVALID_WIDTH_PARAMETER = 
		"image width must be a positive integer";
	
	/** Invalid height error message */
	private static final String INVALID_HEIGHT_PARAMETER = 
		"image height must be a positive integer";
	
	/** Invalid type error message */
	private static final String INVALID_TYPE_PARAMETER = 
		"image type unknown";
	
	/** Invalid buffer error message, null */
	private static final String BUFFER_IS_NULL = 
		"image buffer must be non-null";
	
	/** Invalid buffer error message, insufficient size */
	private static final String BUFFER_INSUFFICIENT = 
		"image buffer must be sufficiently sized to contain image";
	
	/** The image width */
	private int width;
	
	/** The image height */
	private int height;
	
	/** The image format type */
	private int type;
	
	/** The image buffer */
	private ByteBuffer[] buffer;
	
	/** Flag indicating that the image should be treated as grayscale, 
	*  regardless of the actual number of components */
	private boolean isGrayScale;
	
	/**
	 * Constructor
	 *
	 * @param width The image width
	 * @param height The image height
	 * @param type The image format type
	 * @throws IllegalArgumentException if either the width or height arguments
	 * are not positive integers
	 * @throws NullPointerException if the type argument is <code>null</code>
	 */
	public ByteBufferImage ( int width, int height, int type ) {
		this( 
			width, 
			height, 
			type, 
			( type == INTENSITY ) | ( type == INTENSITY_ALPHA ),
			ByteBuffer.allocateDirect( width*height*type ) );
	}
	
	/**
	 * Constructor
	 *
	 * @param width The image width
	 * @param height The image height
	 * @param type The image format type
	 * @param buffer The image data
	 * @throws IllegalArgumentException if either the width or height arguments
	 * are not positive integers
	 * @throws NullPointerException if either the type or buffer argument are 
	 * <code>null</code>
	 * @throws IllegalArgumentException if the buffer is insufficiently sized
	 */
	public ByteBufferImage ( int width, int height, int type, ByteBuffer buffer ) {
		this( 
			width, 
			height, 
			type, 
			( type == INTENSITY ) | ( type == INTENSITY_ALPHA ),
			buffer );
	}
	/**
	 * Constructor
	 *
	 * @param width The image width
	 * @param height The image height
	 * @param type The image format type
	 * @param isGrayScale Flag indicating that the image should be treated as
	 * grayscale, regardless of the format
	 * @param buffer The image data
	 * @throws IllegalArgumentException if either the width or height arguments
	 * are not positive integers
	 * @throws NullPointerException if either the type or buffer argument are 
	 * <code>null</code>
	 * @throws IllegalArgumentException if the buffer is insufficiently sized
	 */
	public ByteBufferImage ( int width, int height, int type, 
		boolean isGrayScale, ByteBuffer buffer ) {
		
		if ( width < 1 ) {
			throw new IllegalArgumentException( INVALID_WIDTH_PARAMETER );
		}
		else if ( height < 1 ) {
			throw new IllegalArgumentException( INVALID_HEIGHT_PARAMETER );
		}
		else if ( ( type < INTENSITY ) || ( type > RGBA ) ) {
			throw new IllegalArgumentException( INVALID_TYPE_PARAMETER );
		}
		else if ( buffer == null ) {
			throw new NullPointerException( BUFFER_IS_NULL );
		}
		else if ( buffer.limit( ) < width * height * type ) {
			throw new IllegalArgumentException( BUFFER_INSUFFICIENT );
		}
		this.width = width;
		this.height = height;
		this.type = type;
		this.isGrayScale = isGrayScale;
		this.buffer = new ByteBuffer[]{ buffer };
	}
	
	/** 
	 * Return the image width
	 *
	 * @return The image width
	 */
	public int getWidth( ) {
		return( width );
	}
	
	/** 
	 * Return the image height
	 *
	 * @return The image height
	 */
	public int getHeight( ) {
		return( height );
	}
	
	/** 
	 * Return the image format type
	 *
	 * @return The image format type
	 */
	public int getType( ) {
		return( type );
	}
	
	/** 
	 * Return whether the image should be treated as grayscale
	 *
	 * @return whether the image should be treated as grayscale
	 */
	public boolean isGrayScale( ) {
		return( isGrayScale );
	}
	
	/** 
	 * Return the number of image levels
	 *
	 * @return The number of image levels
	 */
	public int getLevels( ) {
		return( buffer.length );
	}
	
	/** 
	 * Return the image buffer
	 *
	 * @return The image buffer
	 */
	public ByteBuffer getBuffer( ) {
		buffer[0].rewind( );
		return( buffer[0] );
	}
	
	/** 
	 * Return the image buffer array
	 *
	 * @return The image buffer array
	 */
	public ByteBuffer[] getBuffer( ByteBuffer[] ret_buf ) {
		int size = buffer.length;
		if ( ( ret_buf == null ) || ( ret_buf.length < size ) ) {
			ret_buf = new ByteBuffer[size];
		}
		for ( int i = 0; i < size; i++ ) {
			buffer[i].rewind( );
			ret_buf[i] = buffer[i];
		}
		return( ret_buf );
	}
	
	/** 
	 * Set the image buffer
	 *
	 * @param buffer The image buffer
	 * @throws NullPointerException if the argument buffer is <code>null</code>
	 * @throws IllegalArgumentException if the buffer is insufficiently sized
	 */
	public void setBuffer( ByteBuffer buffer ) {
		if ( buffer == null ) {
			throw new NullPointerException( BUFFER_IS_NULL );
		}
		else if ( buffer.limit( ) < width * height * type ) {
			throw new IllegalArgumentException( BUFFER_INSUFFICIENT );
		}
		this.buffer = new ByteBuffer[]{ buffer };
	}
	
	/** 
	 * Set the image buffer array
	 *
	 * @param buffer The image buffer
	 * @throws NullPointerException if the argument buffer is <code>null</code>
	 * @throws IllegalArgumentException if the buffer is insufficiently sized
	 */
	public void setBuffer( ByteBuffer[] buffer ) {
		if ( buffer == null ) {
			throw new NullPointerException( BUFFER_IS_NULL );
		}
		else if ( buffer[0].limit( ) != width*height*type ) {
			throw new IllegalArgumentException( BUFFER_INSUFFICIENT );
		}
		int size = buffer.length;
		this.buffer = new ByteBuffer[size];
		for ( int i = 0; i < size; i++ ) {
			this.buffer[i] = buffer[i];
		}
	}
	
	/**
	 * Return a description of the image
	 *
	 * @return a description of the image
	 */
	public String toString( ) {
		return( 
			"ByteBufferImage: " +
			"type = " + getTypeDescription( ) +
			", width = " + width + 
			", height = " + height );
	}
	
	/**
	 * Return a description of the image type
	 *
	 * @return a description of the image type
	 */
	private String getTypeDescription( ) {
		switch( type ) {
		case INTENSITY:
			return( "INTENSITY" );
		case INTENSITY_ALPHA:
			return( "INTENSITY_ALPHA" );
		case RGB:
			return( "RGB" );
		case RGBA:
			return( "RGBA" );
		default:
			return( "UNKNOWN" );
		}
	}
}
