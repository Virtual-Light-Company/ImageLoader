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
import java.net.ContentHandler;
import java.net.ContentHandlerFactory;

// Application specific imports
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
 * @author     <A HREF="mailto:justin@vlc.com.au">Justin Couch</A>
 * @version    1.00 30th November 1998
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
      if (mimetype.equalsIgnoreCase("image/png"))
         return new png();
      else if (mimetype.equalsIgnoreCase("image/bmp"))
         return new bmp();
      else if (mimetype.equalsIgnoreCase("image/jpeg"))
         return new jpeg();
      else if (mimetype.equalsIgnoreCase("image/gif"))
         return new gif();
      else if (mimetype.equalsIgnoreCase("image/targa"))
         return new targa();
      else if (mimetype.equalsIgnoreCase("image/x-portable-pixmap"))
         return new x_portable_pixmap();
      else if (mimetype.equalsIgnoreCase("image/x-portable-graymap"))
         return new x_portable_graymap();
      else if (mimetype.equalsIgnoreCase("image/tiff"))
         return new tiff();

      // handle previous content handlers
      if (prevFactory != null)
         return prevFactory.createContentHandler(mimetype);

      // return null if unsure.
      return null;
   }
}

