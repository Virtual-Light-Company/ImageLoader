/*****************************************************************************
 *                     The Virtual Light Company Copyright (c) 1999
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
// none

/**
 * Content handler to load Microsoft Bitmap files.
 * Mime type: image/bmp
 * <P>
 *
 * This softare is released under the
 * <A HREF="http://www.gnu.org/copyleft/lgpl.html">GNU LGPL</A>
 * <P>
 *
 * @author  Justin Couch
 * @version $Revision: 1.3 $
 */
public class bmp extends ContentHandler
{
    /**
     * Given a URL connect stream positioned at the beginning of the BMP
     * image file, this method reads that stream and creates an Image from it.
     *
     * @param u an URL connection.
     * @return the Image, or null on error.
     * @exception IOException if an I/O error occurs while reading the object.
     */
    public Object getContent(URLConnection u)
        throws IOException
    {
        // create a new image decoder ready to decode a BMP image
        ImageBuilder decoder = new ImageBuilder("bmp");

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
        ImageBuilder decoder = new ImageBuilder("bmp");

        Object ret_val = null;

        for(int i = 0; i < classes.length && (ret_val != null); i++)
        {
            int decode_type = -1;

            if(classes[i].isInstance(ImageProducer.class))
            {
                decode_type = ImageBuilder.IMAGEPRODUCER_REQD;
            }
            else if(classes[i].isInstance(Image.class) ||
                    classes[i].isInstance(BufferedImage.class))
            {
                decode_type = ImageBuilder.IMAGE_REQD;
            }
            else if(classes[i].isInstance(WritableRaster.class))
            {
                decode_type = ImageBuilder.WRITABLE_RASTER_REQD;
            }
            else if(classes[i].isInstance(Raster.class))
            {
                decode_type = ImageBuilder.RASTER_REQD;
            }

            if(decode_type != -1)
                ret_val = decoder.decode(u.getInputStream(), decode_type);
        }

        return ret_val;
    }
}

