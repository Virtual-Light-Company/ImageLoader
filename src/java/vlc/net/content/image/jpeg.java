/*****************************************************************************
 *                     The Virtual Light Company Copyright (c) 1999 - 2007
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
import java.awt.image.BufferedImage;
import java.awt.image.ImageProducer;
import java.awt.image.Raster;
import java.awt.image.WritableRaster;

import java.io.IOException;

import java.net.URLConnection;
import java.net.ContentHandler;

// Application specific imports
import vlc.image.ByteBufferImage;

/**
 * Content handler to load Joint Photo Expert Group files.
 * Mime type: image/jpeg
 * <P>
 *
 * This softare is released under the
 * <A HREF="http://www.gnu.org/copyleft/lgpl.html">GNU LGPL</A>
 * <P>
 *
 * @author  Justin Couch
 * @version $Revision: 1.4 $
 */
public class jpeg extends ContentHandler
{
    /**
     * Given a URL connect stream positioned at the beginning of the JPEG
     * image file, this method reads that stream and creates an Image from it.
     *
     * @param u an URL connection.
     * @return the Image, or null on error.
     * @exception IOException if an I/O error occurs while reading the object.
     */
    public Object getContent(URLConnection u)
        throws IOException
    {
        // create a new image decoder ready to decode a JPEG image
        ImageBuilder decoder = new ImageBuilder("jpeg");

        // now decode the image from the input stream
        return decoder.decode(u.getInputStream(), ImageBuilder.IMAGE_REQD);
    }

    /**
     * Given a URL connect stream positioned at the beginning of the
     * representation of an object, this method reads that stream and creates
     * an object that matches one of the types specified. The types are
     * taken in the order specified in the list.
     *
     * @param u an URL connection.
     * @param classes The list of classes to go looking for
     * @return the Image, or null on error.
     * @exception IOException if an I/O error occurs while reading the object.
     */
    public Object getContent(URLConnection u, Class[] classes)
        throws IOException
    {
        // create a new image decoder ready to decode a JPEG image
        ImageBuilder decoder = new ImageBuilder("jpeg");

        Object ret_val = null;

        //for(int i = 0; i < classes.length && (ret_val != null); i++)
		for(int i = 0; i < classes.length; i++)
        {
			Class c = classes[i];
            int decode_type = -1;

            //if(classes[i].isInstance(ImageProducer.class))
			if( c.isAssignableFrom( ImageProducer.class ) )
            {
                decode_type = ImageBuilder.IMAGEPRODUCER_REQD;
            }
            //else if(classes[i].isInstance(Image.class) ||
            //        classes[i].isInstance(BufferedImage.class))
			else if( c.isAssignableFrom( BufferedImage.class ) ||
				c.isAssignableFrom( Image.class ) ) 
            {
                decode_type = ImageBuilder.IMAGE_REQD;
            }
            //else if(classes[i].isInstance(WritableRaster.class))
			else if( c.isAssignableFrom( WritableRaster.class ) )
            {
                decode_type = ImageBuilder.WRITABLE_RASTER_REQD;
            }
            //else if(classes[i].isInstance(Raster.class))
			else if( c.isAssignableFrom( Raster.class ) )
            {
                decode_type = ImageBuilder.RASTER_REQD;
            }
            else if( c.isAssignableFrom( ByteBufferImage.class )) 
            {
                decode_type = ImageBuilder.BYTEBUFFERIMAGE_REQD;
            }
			
            if(decode_type != -1)
                ret_val = decoder.decode(u.getInputStream(), decode_type);
        }

        return ret_val;
    }
}

