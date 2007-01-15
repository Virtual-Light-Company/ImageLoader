
// External imports
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.io.*;
import java.net.*;
import java.util.*;

// Local imports
import vlc.net.content.ImageContentHandlerFactory;
import vlc.net.content.ImageFileNameMap;

/**
 * Test for multiple image loads in parallel to test the multithreaded
 * handling.
 *
 * @author Justin Couch
 * @version $Revision: 1.1 $
 */
public class ParallelViewer extends JFrame
    implements ActionListener
{
    private JPanel imagePanel;

    private JButton button;
    private static boolean useNativeImageLoader;

    public ParallelViewer()
    {
        super("Test Image Viewer");

        setDefaultCloseOperation(EXIT_ON_CLOSE);


        // create a new button and associate event listener
        JButton button = new JButton("load image");
        button.addActionListener(this);

        imagePanel = new JPanel(new FlowLayout());

        // add button to frame
        Container pane = getContentPane();
        pane.setLayout(new BorderLayout());
        pane.add(button, BorderLayout.NORTH);
        pane.add(imagePanel, BorderLayout.CENTER);

        // a nice healthy size
        setSize(500, 500);
        setVisible(true);
    }

    /**
     * Loads an image when the button is pressed.
     */
    public void actionPerformed(ActionEvent e)
    {
        JFileChooser chooser = new JFileChooser(System.getProperty("user.dir"));

        int returnVal = chooser.showOpenDialog(this);
        if(returnVal != JFileChooser.APPROVE_OPTION)
            return;

        try
        {
            File file = chooser.getSelectedFile();
            URL url = file.toURL();
            String filename =  file.getName();

            // we need the extra ^ slash for windoze machines, it doesn't
            // hurt unix though

            setTitle("Parallel Viewer: "+ filename);

            for(int i = 0; i < 10; i++)
            {
                JLabel l = new JLabel();
                imagePanel.add(l);
                ImageLoader loader = new ImageLoader(url, l, useNativeImageLoader);
                Thread th = new Thread(loader);
                th.start();
            }
        }
        catch(MalformedURLException mue)
        {
            System.out.println("error: " + mue.getMessage());
        }

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

        new ParallelViewer();
    }
}
