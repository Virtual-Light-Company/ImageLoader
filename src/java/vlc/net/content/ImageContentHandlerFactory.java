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
import java.net.ContentHandler;
import java.net.ContentHandlerFactory;

// Local imports
import vlc.net.content.image.*;

/**
 * Content Handler factory used to load user defined content handlers,
 * for images.
 * This is needed because jdk1.2 does not seem to work when defining
 * the property 'java.content.handler.pkgs'.  Actually, this content
 * handler factory is needed as MIME types containing a hyphen will
 * not be able to be used as the hyphen is not a legal identifier
 * character.
 * <P>
 * This softare is released under the
 * <A HREF="http://www.gnu.org/copyleft/lgpl.html">GNU LGPL</A>
 * <P>
 *
 * @author Justin Couch
 * @version $Revision: 1.3 $
 */
public class ImageContentHandlerFactory implements ContentHandlerFactory
{
   /** previous factory */
   private ContentHandlerFactory prevFactory;

   /**
    * Constructor.
    * Constructs a new content handler factory for determining
    * image content handlers.
    */
   public ImageContentHandlerFactory()
   {
     prevFactory = null;
   }

   /**
    * Constructor.
    * This will chain this factory to the given factory.
    * Processing in this factory will have preference
    * to the previous factories.
    * @param factory previous content handler factory
    */
   public ImageContentHandlerFactory(ContentHandlerFactory factory)
   {
      prevFactory = factory;
   }

   /**
    * Creates a new ContentHandler to read an object from a URLStreamHandler.
    * @param mimetype the MIME type for which a content handler is desired.
    * @return a new ContentHandler to read an object from a URLStreamHandler,
    * or null if the mimetype is not understood.
    */
   public ContentHandler createContentHandler(String mimetype)
   {
      ContentHandler ret_val = null;

      if (mimetype.equalsIgnoreCase("image/png"))
         ret_val = new png();
      else if (mimetype.equalsIgnoreCase("image/bmp"))
         ret_val = new bmp();
      else if (mimetype.equalsIgnoreCase("image/jpeg"))
         ret_val = new jpeg();
      else if (mimetype.equalsIgnoreCase("image/gif"))
         ret_val = new gif();
      else if (mimetype.equalsIgnoreCase("image/targa"))
         ret_val = new targa();
      else if (mimetype.equalsIgnoreCase("image/x-portable-pixmap"))
         ret_val = new x_portable_pixmap();
      else if (mimetype.equalsIgnoreCase("image/x-portable-graymap"))
         ret_val = new x_portable_graymap();
      else if (mimetype.equalsIgnoreCase("image/tiff"))
         ret_val = new tiff();

      // handle previous content handlers
      if (ret_val == null && prevFactory != null)
         ret_val = prevFactory.createContentHandler(mimetype);

      // return null if unsure.
      return ret_val;
   }
}

