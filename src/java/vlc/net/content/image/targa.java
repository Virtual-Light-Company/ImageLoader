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

package vlc.net.content.image;

// Standard imports
import java.net.*;
import java.io.*;

// Application specific imports
//none

/**
 * Content handler to load Targa Graphics files.
 * Mime type: image/targa
 * <P>
 *
 * This softare is released under the
 * <A HREF="http://www.gnu.org/copyleft/lgpl.html">GNU LGPL</A>
 * <P>
 *
 * @author     <A HREF="mailto:justin@vlc.com.au">Justin Couch</A>
 * @version    1.00 30th November 1998
 */
public class targa extends ContentHandler
{
   /**
    * Given a URL connect stream positioned at the beginning of the TARGA
    * image file, this method reads that stream and creates an Image from it.
    *
    * @param u an URL connection.
    * @return the Image, or null on error.
    * @exception IOException if an I/O error occurs while reading the object.
    */
   public Object getContent(URLConnection u)
      throws IOException
   {
      // create a new image decoder ready to decode a Targa image
      ImageDecoder decoder = new ImageDecoder("targa");

      // now decode the image from the input stream
      return decoder.decode(u.getInputStream());
   }
}

