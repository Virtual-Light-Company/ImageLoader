
// Standard imports
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.net.*;
import java.util.*;

import java.awt.image.ImageProducer;

// Application specific imports
import vlc.net.content.image.ImageBuilder;

/**
 * This is a test harness to test the content handler for loading images.
 * Note the yucky hack to work around a java bug with FileNameMap in main()
 *
 * @author      <A HREF="mailto:justin@vlc.com.au">Justin Couch</A>,
 * @version     1.00 20th August 1998
 */
public class RawViewer extends Frame
    implements ActionListener, WindowListener
{
    private Image img = null;
    private Button button;
    private long startTime;
    private boolean timingDone;
    private static boolean useNativeImageLoader;
    private FileDialog dialog;


    public RawViewer()
    {
        super("Test Raw Viewer");
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
        String file_name =  dialog.getDirectory() + dialog.getFile();

        setTitle("Test Image Viewer: "+dialog.getFile());

        // record time we start the loading
        startTime = System.currentTimeMillis();
        timingDone = false;

        // load it!
        loadImage(file_name);

        repaint();
    }

    /**
     * Does the actual loading of the image.
     * @param urlString the url of the image to load
     */
    private void loadImage(String fileName)
    {
        // if a previous image is loaded, ensure it will get garbage collected
        if (img != null)
        {
            img.flush();
            img = null;
        }

        try
        {
            String type = null;

            if(fileName.toUpperCase().endsWith(".PNG"))
                type = "png";
            else if(fileName.toUpperCase().endsWith(".TIF"))
                type = "tiff";
            else if(fileName.toUpperCase().endsWith(".TIFF"))
                type = "tiff";
            else if(fileName.toUpperCase().endsWith(".TGA"))
                type = "targa";
            else if(fileName.toUpperCase().endsWith(".BMP"))
                type = "bmp";
            else if(fileName.toUpperCase().endsWith(".PPM"))
                type = "ppm";
            else if(fileName.toUpperCase().endsWith(".PGM"))
                type = "pgm";
            else if(fileName.toUpperCase().endsWith(".JPG"))
                type = "jpeg";

            ImageBuilder decoder = new ImageBuilder(type);

            // now use our content handlers
            Object o = decoder.decode(new FileInputStream(fileName),
                                      ImageBuilder.IMAGEPRODUCER_REQD);
//                                      ImageBuilder.IMAGE_REQD);

            if(o instanceof ImageProducer)
            {
                img = Toolkit.getDefaultToolkit().createImage((ImageProducer)o);
                MediaTracker tkr = new MediaTracker(this);
                try
                {
                    tkr.addImage(img, 0);
                    tkr.waitForAll();
                }
                catch(Exception e)
                {
                }
            }
            else
            {
                // not our content handler
                img = createImage(500, 300);
                Graphics g = img.getGraphics();
                g.drawString("No content handler for : "+fileName, 50, 150);
                g.dispose();
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

        new RawViewer();
    }
}

