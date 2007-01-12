/*****************************************************************************
 *                The Virtual Light Company Copyright (c) 1999 - 2007
 *                               Java Source
 *
 * This code is licensed under the GNU Library GPL v2.1. Please read license.txt
 * for the full details. A copy of the LGPL may be found at
 *
 * http://www.gnu.org/copyleft/lgpl.html
 *
 ****************************************************************************/

package vlc.net.content;


// External imports
import java.net.*;
import java.io.IOException;

// Local imports
// None

/**
 * This class provides functionality for determine MIME types
 * based on image file names.  For example, a filename of
 * "santa.jpg" would return the MIME type "image/jpeg".
 * <P>
 * <code>FileNameMap</code>s can be chained together in much
 * the same way as java IOStreams.
 * <P>
 * This softare is released under the
 * <A HREF="http://www.gnu.org/copyleft/lgpl.html">GNU LGPL</A>
 * <P>
 *
 * @author Justin Couch
 * @version $Revision: 1.3 $
 */
public class ImageFileNameMap
    implements FileNameMap
{
    /** previous filename map */
    private FileNameMap prevMap;

    /**
    * Constructor.
    * Constructs a new filename map for handling images.
    */
    public ImageFileNameMap()
    {
        prevMap = null;
    }

    /**
     * Constructor.
     * This will chain this filename map to the given map.
     * Processing in this filename map will have preference
     * to the previous maps.
     * @param map previous filename maps
     */
    public ImageFileNameMap(FileNameMap map)
    {
        prevMap = map;
    }

    /**
     * Returns the content type for the file based on the
     * given filename.  The content type is determined
     * from the filename extension.
     * @param fileName name of the file.
     * @return MIME content type of this filename, or
     * null if the content type is unknown
     */
    public String getContentTypeFor(String fileName)
    {
        String ret_val = null;

        if(fileName.toUpperCase().endsWith(".PNG"))
            ret_val = "image/png";
        else if(fileName.toUpperCase().endsWith(".TIF"))
            ret_val = "image/tiff";
        else if(fileName.toUpperCase().endsWith(".TIFF"))
            ret_val = "image/tiff";
        else if(fileName.toUpperCase().endsWith(".TGA"))
            ret_val = "image/targa";
        else if(fileName.toUpperCase().endsWith(".BMP"))
            ret_val = "image/bmp";
        else if(fileName.toUpperCase().endsWith(".PPM"))
            ret_val = "image/x-portable-pixmap";
        else if(fileName.toUpperCase().endsWith(".PGM"))
            ret_val = "image/x-portable-graymap";

        // handle previous filename maps
        if(ret_val == null && prevMap != null)
            return prevMap.getContentTypeFor(fileName);

        // return null if unsure.
        return ret_val;
    }
}
