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
import java.awt.image.*;

/**
 * This is a buffer that contains rectangular data that is to represent an
 * image.  This class implements the ImageProducer interface to allow other
 * classes to obtain an image from this data easily.
 * <P>
 *
 * This softare is released under the
 * <A HREF="http://www.gnu.org/copyleft/lgpl.html">GNU LGPL</A>
 *
 * @author  Justin Couch
 * @version $Revision: 1.3 $
 */
class ImageBuffer
    implements ImageProducer
{
    /** size of chunk to allocate */
    private static final int MEM_LIMIT = 3000000;

    /** maximum number of allowed images consumers */
    private static final int MAX_CONSUMERS = 20;

    /** The common colour model for all uses */
    private ColorModel colorModel;

    /** List of current registered consumers */
    private ImageConsumer[] consumers;

    /** Number of currently registered consumers */
    private int count;

    /** The image data in chunks */
    private int[][] data;

    /** Number of rows contained in each chunk of the data */
    private int[] rowsForChunk;

    /** Width of the image */
    private int width = 0;

    /** Height of the image */
    private int height = 0;

    /** The ColorModel to be used */
    private ColorModel model = null;

    /**
     * Creates an empty buffer of the specified dimensions.
     *
     * @param width The image width
     * @param height The image height
     */
    ImageBuffer(int width, int height, int numComponents)
    {
        // create the default ColorModel
        switch(numComponents) {
            case 1:
                colorModel = new DirectColorModel(8, 0, 0, 0xFF, 00);
                break;

            case 2:
                colorModel = new DirectColorModel(16, 0, 0xFF00, 0xFF, 0);
                break;

            case 3:
                colorModel =
                    new DirectColorModel(24, 0xFF0000, 0xFF00, 0xFF, 0);

                break;

            case 4:
                colorModel =
                    new DirectColorModel(32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
                break;
        }

        // set image dimensions
        this.width = width;
        this.height = height;

        // allocate image consumers array
        consumers = new ImageConsumer[MAX_CONSUMERS];
        count = 0;

        // allocate memory to store the image data
        allocateBuffer();
    }

    /**
     * Sets the pixel data for the given row.  Each pixel must be represented
     * by an integer of the form ARGB.
     * @param row the row number of the data to set.
     * @param rowData a rows worth of image data.  If this array is less than
     * the ImageBuffer width, an exception will be thrown.  If larger, excess
     * data will be ignored.
     * @exception IllegalArgumentException if rowData array passed was too small,
     * or the given row was invalid.
     */
    void setImageRow(int row, int[] rowData)
    {
        // ensure row is valid
        if((row < 0) || (row > height))
            throw new IllegalArgumentException("ImageBuffer.setImageRow(): "+
                                                          "invalid row: "+row);

        // ensure data is valid
        if(rowData.length < width)
            throw new IllegalArgumentException("ImageBuffer.setImageRow(): "+
                                                          "not enough data");

        // find the chunk, and offset to place the data
        int currentChunk = 0;
        int offset = 0;
        int minRows = 0;
        int maxRows = 0;
        boolean found = false;

        do
        {
            minRows = maxRows;
            maxRows += rowsForChunk[currentChunk];

            if((row >= minRows) && (row < maxRows))
            {
                found = true;
                offset = (row - minRows) * width;
            }
            else
                currentChunk++;
        }
        while(!found);

        // now copy the data
        System.arraycopy(rowData, 0, data[currentChunk], offset, width);
    }

    /**
     * This method is used to register an ImageConsumer with the ImageProducer
     * for access to the image data during a later reconstruction of the Image.
     * The ImageProducer may, at its discretion, start delivering the image data
     * to the consumer using the ImageConsumer interface immediately, or when
     * the next available image reconstruction is triggered by a call to the
     * startProduction method.
     * @param ic the ImageConsumer to be added.
     */
    public void addConsumer(ImageConsumer ic)
    {
        // ensure we don't add a consumer more than once
        if(isConsumer(ic))
            return;

        // check if we have to reallocate space for more consumers
        if(count >= consumers.length-1)
        {
            // create larger array
            ImageConsumer[] tmp = new ImageConsumer[count + MAX_CONSUMERS];

            // copy across old consumers
            for(int i = 0; i < count; i++)
                tmp[i] = consumers[i];

            // update consumer reference
            consumers = tmp;
        }

        // add new ImageConsumer to list
        consumers[count++] = ic;
    }

    /**
     * This method determines if a given ImageConsumer object is currently
     * registered with this ImageProducer as one of its consumers.
     * @param ic the ImageConsumer to be tested.
     * @return true if it is a consumer, false if it is not.
     */
    public boolean isConsumer(ImageConsumer ic)
    {
        for(int i = 0; i < count; i++)
        {
            if(consumers[i] == ic)
                return true;
        }

        return false;
    }

    /**
     * This method removes the given ImageConsumer object from the list of
     * consumers currently registered to receive image data. It is not
     * considered an error to remove a consumer that is not currently
     * registered. The ImageProducer should stop sending data to this consumer
     * as soon as is feasible.
     * @param the ImageConsumer to be removed.
     */
    public void removeConsumer(ImageConsumer ic)
    {
        int i = 0;

        // find the index of the consumer to be removed
        for(i = 0; i < count; i++)
        {
            if(consumers[i] == ic)
            {
                consumers[i] = null;
                break;
            }
        }

        // copy any remaining consumers over the top
        for(int j = i; j < (count - 1); j++)
            consumers[i] = consumers[i + 1];

        // change count
        count--;
    }

    /**
     * This method both registers the given ImageConsumer object as a consumer
     * and starts an immediate reconstruction of the image data which will then
     * be delivered to this consumer and any other consumer which may have
     * already been registered with the producer.  This method differs from the
     * addConsumer method in that a reproduction of the image data should be
     * triggered as soon as possible.
     * @param ic the ImageConsumer to be added.
     */
    public void startProduction(ImageConsumer ic)
    {
        // add this consumer
        addConsumer(ic);

        int i;

        // set the dimensions of the image to all registered consumers
        for(i = 0; i < count; i++)
            consumers[i].setDimensions(width , height);

        // set the ColorModel to be used
        for(i = 0; i < count; i++)
            consumers[i].setColorModel(colorModel);

        // set the hints to let all consumers know that the data will be sent
        // in top down left to right order
        for(i = 0; i < count; i++)
            consumers[i].setHints(ImageConsumer.SINGLEFRAME);

        // current row we are sending
        int thisRow = 0;
        int numChunks = rowsForChunk.length;

        // send the pixel data now
        // send the data now one row at a time
        for(i = 0; i < numChunks; i++)
        {
            for(int j = 0; j < count; j++)
            {
                consumers[j].setPixels(0,
                                       thisRow,
                                       width,
                                       rowsForChunk[i],
                                       colorModel,
                                       data[i],
                                       0,
                                       width);
            }
            thisRow += rowsForChunk[i];
        }

        // send the message that the
        for(i = 0; i < count; i++)
            consumers[i].imageComplete(ImageConsumer.SINGLEFRAMEDONE);
    }

    /**
     * This method is used by an ImageConsumer to request that the ImageProducer
     * attempt to resend the image data one more time in TOPDOWNLEFTRIGHT order
     * so that higher quality conversion algorithms which depend on receiving
     * pixels in order can be used to produce a better output version of the
     * image. The ImageProducer is free to ignore this call if it cannot resend
     * the data in that order. If the data can be resent, then the ImageProducer
     * should respond by executing the following minimum set of ImageConsumer
     * method calls:<br>
     *          ic.setHints(TOPDOWNLEFTRIGHT | < otherhints >);<br>
     *          ic.setPixels(...);        // As many times as needed<br>
     *          ic.imageComplete();
     * @param the Image Producer requesting.
     */
    public void requestTopDownLeftRightResend(ImageConsumer ic)
    {
        // set the dimensions of the image
        ic.setDimensions(width , height);

        // set the ColorModel to be used
        ic.setColorModel(colorModel);

        // set the hints to let all consumers know that the data will be sent
        // in top down left to right order
        ic.setHints(ImageConsumer.TOPDOWNLEFTRIGHT);

        // current row we are sending
        int thisRow = 0;
        int numChunks = rowsForChunk.length;

        // send the pixel data now
        // send the data now one row at a time
        for(int i = 0; i < numChunks; i++)
        {
            ic.setPixels(0,
                         thisRow,
                         width,
                         rowsForChunk[i],
                         colorModel,
                         data[i],
                         0,
                         width);
            thisRow += rowsForChunk[i];
        }

        // send the message that the
        ic.imageComplete(ImageConsumer.SINGLEFRAMEDONE);
    }


    /**
     * Allocates enough memory to hold an image (width * height integers).  This
     * memory is allocated in chunks, where each chunk contain space for
     * multiple rows of image data.  The aim is to have as few chunks as
     * possible (hence large chunks), to keep the number of Object handles used
     * to a minimum.  This is necessary for large images.
     * <p>
     * We can not just allocate one big contiguous 1 dimensional array, as
     * this approach fails all too often with large images.
     */
    private void allocateBuffer()
    {
        // assume no more than 100 chunks, but can allocate more if needed
        int[][] chunkPtrs = new int[100][];

        int numPtrs = 100;

        int rowsLeft = height;

        int numChunks = 0;
        int rowsAllocated = 0;
        int[] oneChunk = null;

        // now allocate all the chunks
        do
        {
            int memNeeded = width * rowsLeft;
            int memory_limit = MEM_LIMIT;

            // allocate memory for one chunk
            boolean failed;
            do
            {
                failed = false;
                try
                {
                    if(memNeeded > memory_limit)
                    {
                        rowsAllocated = memory_limit / width;
                        oneChunk = new int[rowsAllocated * width];
                    }
                    else
                    {
                        rowsAllocated = rowsLeft;
                        oneChunk = new int[memNeeded];
                    }
                }
                catch(OutOfMemoryError e)
                {
                    failed = true;
                    // reduce memory by 1/3 and try again
                    memory_limit = memory_limit * 2 / 3;
                }
            }
            while(failed);

            // add chunk to array
            chunkPtrs[numChunks++] = oneChunk;

            // how many rows are left?
            rowsLeft -= rowsAllocated;

            // do we need more chunk pointers?
            if(numChunks >= numPtrs)
            {
                // create a new larger array
                numPtrs += 100;
                int[][] tmp = new int[numPtrs][];

                // copy over elements
                for(int i = 0; i < numChunks; i++)
                    tmp[i] = chunkPtrs[i];

                // reassign chunkPtrs
                chunkPtrs = tmp;
            }

        }
        while(rowsLeft > 0);

        // create final array of references to chunks
        data = new int[numChunks][];

        for(int i = 0; i < numChunks; i++)
            data[i] = chunkPtrs[i];

        // now create array containing the number of rows in each chunk
        rowsForChunk = new int[numChunks];

        // count number of rows in each chunk
        for(int i = 0; i < numChunks; i++)
            rowsForChunk[i] = data[i].length / width;
    }
}

