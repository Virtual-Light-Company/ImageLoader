
// Standard imports
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.net.*;
import java.util.*;

// Application specific imports
import vlc.net.content.ImageContentHandlerFactory;
import vlc.net.content.ImageFileNameMap;

/**
 * This is a test harness to test the content handler for loading images.
 * Note the yucky hack to work around a java bug with FileNameMap in main()
 *
 * @author     <A HREF="mailto:justin@vlc.com.au">Justin Couch</A>,
 * @version    1.00 20th August 1998
 */
public class ImageViewer extends Frame
   implements ActionListener, WindowListener
{
   private Image img = null;
   private Button button;
   private long startTime;
   private boolean timingDone;
   private static boolean useNativeImageLoader;
   private FileDialog dialog;


   public ImageViewer()
   {
      super("Test Image Viewer");
      // set layout
      setLayout(new BorderLayout());

      // create a new button and associate event listener
      button = new Button("load image");
      button.addActionListener(this);

      // add button to frame
      add(button, "North");

      // so we can catch window close events
      addWindowListener(this);

      // let the user select an image file to view
      dialog = new FileDialog(this, "Select Image file",
                                         FileDialog.LOAD);
      dialog.setModal(true);
      dialog.setVisible(false);

      // a nice healthy size
      setSize(500, 500);
      setVisible(true);
   }

   public void paint(Graphics g)
   {
      // draw image if it has been loaded
      if (img != null)
      {
         // printout timing info
         if (!timingDone)
         {
            timingDone = true;
            long diff = System.currentTimeMillis() - startTime;
            System.out.println(" time taken in milliseconds: "+diff);
         }

         g.drawImage(img, 1, 50, this);
      }
   }

   /**
    * Loads an image when the button is pressed.
    */
   public void actionPerformed(ActionEvent e)
   {
      dialog.setFile(null);
      dialog.show();

      if (dialog.getFile() == null)
         return;

      // obtain the url to the image
      String url = "file:///" + dialog.getDirectory() + dialog.getFile();
      // we need the extra ^ slash for windoze machines, it doesn't
      // hurt unix though

      setTitle("Test Image Viewer: "+dialog.getFile());

      // record time we start the loading
      startTime = System.currentTimeMillis();
      timingDone = false;

      // load it!
      loadImage(url);

      repaint();
   }

   /**
    * Does the actual loading of the image.
    * @param urlString the url of the image to load
    */
   private void loadImage(String urlString)
   {
      // if a previous image is loaded, ensure it will get garbage collected
      if (img != null)
      {
         img.flush();
         img = null;
      }

      URL url = null;

      try
      {
         // create an URL to the object and open a connection
         url = new URL(urlString);
         URLConnection connection = url.openConnection();

         // get content type
         String contentType = connection.getContentType();

         if (contentType.equals("content/unknown"))
         {
            // unknown content type
            img = createImage(500, 300);
            Graphics g = img.getGraphics();
            g.drawString("Unknown mime type for : "+urlString, 50, 150);
            g.dispose();
         }
         else if (useNativeImageLoader &&
                  (contentType.equals("image/jpeg") ||
                   contentType.equals("image/gif")))
         {
            // let java handle these natively
            img = Toolkit.getDefaultToolkit().getImage(url);

            MediaTracker tracker = new MediaTracker(this);
            try
            {
               tracker.addImage(img, 0);
               tracker.waitForID(0);
            }
            catch (InterruptedException e)
            {
               System.out.println("INTERRUPTED while loading Image");
            }

            if (tracker.isErrorAny())
            {
               System.err.println("Error loading image " + urlString);
            }

            tracker.removeImage(img);
            tracker = null;
         }
         else
         {
            // now use our content handlers
            Object o = connection.getContent();
            if (o instanceof Image)
            {
               img = (Image) o;
            }
            else
            {
               // not our content handler
               img = createImage(500, 300);
               Graphics g = img.getGraphics();
               g.drawString("No content handler for : "+urlString, 50, 150);
               g.dispose();
            }
         }
      }
      catch(MalformedURLException e)
      {
        System.out.println("error: "+e.getMessage());
      }
      catch(IOException e1)
      {
        System.out.println("error: "+e1.getMessage());
      }
   }

   /**
    * So closing the window exists the application.
    */
   public void windowClosing(WindowEvent e)
   {
      setVisible(false);
      System.exit(0);
   }

   // only care about closing the window
   public void windowOpened(WindowEvent e)
   {
   }

   public void windowClosed(WindowEvent e)
   {
   }

   public void windowIconified(WindowEvent e)
   {
   }

   public void windowDeiconified(WindowEvent e)
   {
   }

   public void windowActivated(WindowEvent e)
   {
   }

   public void windowDeactivated(WindowEvent e)
   {
   }

   public static void main(String[] args)
   {
      if (args.length > 0)
      {
         useNativeImageLoader = true;
         System.out.println("\nUsing NATIVE image loaders for jpeg and gif");
      }
      else
      {
         System.out.println("\nUsing CUSTOM image loaders for jpeg and gif");
      }

      // we need to load the content handler factory so our content
      // handler can be found
      URLConnection.setContentHandlerFactory(new ImageContentHandlerFactory());

      // /* JDK1.1 - uncomment this

      // If you are using JDK1.1 you will need to comment out this
      // section as FileNameMap is from jdk1.2.
      // to set up the content handler correctly, you will need to
      // add the following lines to you content-types.properties file.
      //
      // Image/png: \
      //  description=PNG Image;\
      //  file_extensions=.png
      //
      // Image/targa: \
      //  description=Targa Image;\
      //  file_extensions=.tga
      //
      // Image/tiff: \
      //  description=Tiff Image;\
      //  file_extensions=.tif,.tiff
      //
      // Image/bmp: \
      //  description=MS Bitmap Image;\
      //  file_extensions=.bmp

      // stupid workaround for java FileNameMap bug
      try
      {
         // we have to open a file with a known mime type first
         URL url = new URL("file:///c|/winnt/media/chord.wav");
         URLConnection connection = url.openConnection();
         connection.getContentType();
      }
      catch(IOException e)
      {
         System.err.println("error: "+e.getMessage());
      }

      // need to load our own filename map for our
      // handled filetypes
      URLConnection.setFileNameMap(new ImageFileNameMap());

      //  JDK1.1 - uncomment this */

      new ImageViewer();
   }
}

