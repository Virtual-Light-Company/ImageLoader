/*****************************************************************************
 *                The Virtual Light Company Copyright (c) 1999
 *                               Java Source
 *
 * This code is licensed under the GNU Library GPL. Please read license.txt
 * for the full details. A copy of the LGPL may be found at
 *
 * http://www.gnu.org/copyleft/lgpl.html
 *
 * Project:    Image Content Handlers
 *
 * Version History
 * Date        TR/IWOR  Version  Programmer
 * ----------  -------  -------  ------------------------------------------
 *
 ****************************************************************************/

package vlc.net.content;


// Standard imports
import java.net.*;
import java.io.IOException;

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
 * @author     <A HREF="mailto:justin@vlc.com.au">Justin Couch</A>
 * @version    1.00 30th November 1998
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
    if(fileName.toUpperCase().endsWith(".PNG"))
      return "image/png";
    else if(fileName.toUpperCase().endsWith(".TIF"))
      return "image/tiff";
    else if(fileName.toUpperCase().endsWith(".TIFF"))
      return "image/tiff";
    else if(fileName.toUpperCase().endsWith(".TGA"))
      return "image/targa";
    else if(fileName.toUpperCase().endsWith(".BMP"))
      return "image/bmp";

    // handle previous filename maps
    if (prevMap != null)
      return prevMap.getContentTypeFor(fileName);

    // return null if unsure.
    return null;
  }
}
