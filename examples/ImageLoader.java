
// External imports
import java.awt.*;
import javax.swing.*;
import java.io.*;
import java.net.*;

// Local imports
import vlc.net.content.ImageContentHandlerFactory;
import vlc.net.content.ImageFileNameMap;

/**
 * This is a test harness to test the content handler for loading images.
 * Note the yucky hack to work around a java bug with FileNameMap in main()
 *
 * @author Justin Couch
 * @version $Revision: 1.1 $
 */
class ImageLoader implements Runnable
{
    private URL imageURL;
    private JLabel imageLabel;
    private boolean useNativeImageLoader;

    ImageLoader(URL url, JLabel label, boolean useNative)
    {
        imageURL = url;
        imageLabel = label;
        useNativeImageLoader = useNative;
    }

    public void run()
    {
        Image image = null;

        try
        {
            // create an URL to the object and open a connection
            URLConnection connection = imageURL.openConnection();

            // get content type
            String contentType = connection.getContentType();

            if(contentType.equals("content/unknown"))
            {
                // unknown content type
                imageLabel.setText("Unknown mime type for : " + imageURL);
            }
            else if(!useNativeImageLoader)
            {
                // let java handle these natively
                image = Toolkit.getDefaultToolkit().getImage(imageURL);

                MediaTracker tracker = new MediaTracker(imageLabel);
                try
                {
                    tracker.addImage(image, 0);
                    tracker.waitForID(0);
                }
                catch (InterruptedException e)
                {
                    System.out.println("INTERRUPTED while loading Image");
                }

                if (tracker.isErrorAny())
                {
                    System.err.println("Error loading image " + imageURL);
                }

                tracker.removeImage(image);
                tracker = null;
            }
            else
            {
                // now use our content handlers
                Object o = connection.getContent();
                if (o instanceof Image)
                {
                    image = (Image) o;
                }
                else
                {
                    // not our content handler
                    imageLabel.setText("No content handler for : " + imageURL);
                }
            }
        }
        catch(IOException e1)
        {
            System.out.println("I/O error: "+e1.getMessage());
        }

        if(image != null)
            imageLabel.setIcon(new ImageIcon(image));
    }
}
