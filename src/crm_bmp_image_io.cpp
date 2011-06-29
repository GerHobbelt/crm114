//  crm_bit_entropy.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2007 William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.
//
//  Other licenses may be negotiated; contact the
//  author for details.
//
//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"

#include "crm_bmp_prof.h"


/*
 * Code to create and write BMP pictures.
 *
 * These can be used to analyze the algorithmic performance of specific parts of CRM114.
 *
 * For instance, you could analyze the distribution of generated feature hashes by plotting them
 * in a picture, where each pixel represents a certain range of hash values (one pixel per hash would mean we'd need
 * 4 BIGAuint8_t for this assist piture alone, which is not doable on 32-bit machines. Besides, even when you would be able to
 * produce such a image on a 64-bit box, there are very few applications out there that would be able to handle
 * the picture and display it.
 *
 * Generally, depending on your screen, I suggest you limit your BMP to the 2K size (2048x2048). For 'view all at once'
 * I suggest 1K size as a reasonable format which will fit on modern displays without the need to scroll.
 *
 * Pictures will start all-black (zeroed); edits may be 'set' (overwrite pixel value) or 'additive (increment pixel value:
 * colors are brightened smoothly, unless you specifically increment only a single color channel).
 *
 * As the image does not support 'overbright colors' like analog film, we will limit the brightest color to pure bright.
 * Any additional increments will not change this anymore: the 'counters' of each pixel will not 'wrap around'.
 *
 * (In a future release, we may consider using 64-bit counters to circumvent this 'overflow' issue. There's also the option
 * to 'scale' the picture to make sure the colors are 'balanced' so that the highest count equals pure white, while the
 * other counters are 'scaled' as needed.)
 *
 * Axes X and Y depend on your need and the item under analysis.
 *
 * Suggestions:
 *
 *
 *
 * hash distribution @ 1K:
 *
 * this means the 32-bit hashes will be forced into a 1K x 1K range, resulting in a clustering of 4K hashes per pixel.
 *
 *
 * hash 'spread' over time (to help analysis of hash distribution over multiple samples / time):
 *
 * X is time axis (time counter; this means the maximum amount of time must be specified up front so we can determine the
 * 'clustering' per vertical line); Y for hash value.
 *
 *
 * hash spread over time v2.0
 *
 * X as time; Y for hash log(DELTA), i.e. the delta between subsequent values is calculated and the log() values is used
 * to calculate the Y position. As a DELTA is a +/- value, log(0) will be at middle height; negative values are plot
 * below that value as Y' = log(-delta), while positive deltas are plotted above that line at Y'=log(+delta).
 *
 * To determine the scale of Y, the max Y is determined at log(2^31). Using log2() instead of log() for everything in this
 * mode simplifies this scale, as the range will never surpass [-31 .. +31] for 32-bit absolute values.
 *
 *
 * hash table distribution
 *
 * split hash table size over a square X by Y. green is for 'direct hits'; red for chained features. blue could be used for
 * hits by 'classify' instead of 'learn', so often sampled items may become closer to white (hot).
 *
 *
 */




/*
 * Create an allocated analysis image block for further use. To ease the implementation,
 * you, the user, have to fill the appropriate fields in the input INFOBLOCK.
 *
 * Note that you can discard that block when this routine returns as the content will be
 * been copied into the reurned (allocated) result.
 *
 * You can simply call stdlib free() to release the memory used by the analysis.
 */
CRM_ANALYSIS_INFOBLOCK *crm_create_analysis_image(CRM_ANALYSIS_INFOBLOCK *info)
{
    int size;
    CRM_ANALYSIS_INFOBLOCK *p;
    analysis_counter_t *c;

    if (!info)
    {
        return NULL;
    }

    size = info->x_width * info->y_height;
    if (!size)
    {
        return NULL;
    }
    size *= sizeof(info->counters[0]);
    size += sizeof(*info);

    p = (CRM_ANALYSIS_INFOBLOCK *)malloc(size);      // intentionally a 'malloc()':
    // 'image' is zeroed? hm... calloc() zets to zeroed uint8_ts, but floating point 0.0 is something else,
    // so do this by hand and do without the calloc().
    if (!p)
    {
        return NULL;
    }

    *p = *info;
    p->counters = c = (analysis_counter_t *)&p[1];
    p->counterspace_size = p->x_width * p->y_height;

    // zero ('blacken') the image block
    for (size = p->counterspace_size; --size >= 0;)
        c[size] = 0.0;

    p->x_divisor = p->time_range / p->x_width;

    // precalc further intermediates:

    return p;
}



// range limit, offset, then 'plot' item into grid.
void crm_img_analyze_value(CRM_ANALYSIS_INFOBLOCK *info, double value)
{
    if (!info)
        return;

    // plot process:

    // 1: range limiting:
    if (value > info->minimum_value)
    {
        value = info->minimum_value;
    }
    else if (value < info->minimum_value)
    {
        value = info->minimum_value;
    }

    // 2: apply offset: ALWAYS do this, even when offset is zero (0.0)
    value += info->offset_for_data;

    // 3: apply some generic mode flags:
    // unsigned int take_delta: 1;  // use the sample delta instead of the absolute input value
    if (info->take_delta)
    {
        double delta = value - info->previous_value;
        info->previous_value = value;
        value = delta;

        // unsigned int plot_bidir: 1;   // plot zero (= 'offset_for_data') at middle Y position
        //
        // if NOT set, all negative values will be lost, so we take abs(delta) then to make them reappear in + space.
        if (info->plot_bidir == 0 && value < 0.0)
        {
            value = -value;
        }
    }

    // unsigned int data_is_log2: 1;    // plot the data in log2() instead of linear
    //
    // This bit of code makes sure the sign of the value is kept intact while
    // the abs(value) is pulled through a log2() operation.
    if (info->data_is_log2)
    {
        if (value > DBL_EPSILON)
        {
            value = log2(value);
            value += info->log2_shift;
            // clip off negative part of log2() curve.
            if (value < 0.0)
            {
                value = 0.0;
            }
        }
        else if (value < -DBL_EPSILON)
        {
            value = log2(-value);
            value += info->log2_shift;
            // clip off negative part of log2() curve.
            if (value < 0.0)
            {
                value = 0.0;
            }
            value = -value;
        }
        else
        {
            value = 0.0;
        }
    }


    // 4: plot the data in the image store:
    //
    // unsigned int x_is_time: 1; // y is data range; x is time range - if not set, data is spread over the square of X by Y
    if (info->x_is_time)
    {
        int x_time;
        int y_pos;

        if (info->current_time >= info->time_range)
        {
            // time range expired; do not add to the plot anymore.
            return;
        }
        x_time = info->current_time++;

        x_time /= info->x_divisor;
        CRM_ASSERT(x_time >= 0);
        CRM_ASSERT(x_time < info->x_width);

        value *= info->y_single_scale;

        // unsigned int plot_bidir: 1;   // plot zero (= 'offset_for_data') at middle Y position
        //
        // This one is already done in the precalcs
        value += info->y_middle_offset;

        y_pos = (int)value;
        // allow for a little floating point calc error:
        CRM_ASSERT(y_pos >= -1);
        CRM_ASSERT(y_pos <= info->y_height);
        if (y_pos < 0)
        {
            y_pos = 0;
        }
        else if (y_pos >= info->y_height)
        {
            y_pos = info->y_height - 1;
        }

        // plot value in additive mode.
        y_pos += x_time * info->y_height;
        CRM_ASSERT(y_pos >= 0);
        CRM_ASSERT(y_pos < info->counterspace_size);
        info->counters[y_pos]++;
    }
    else
    {
        // square X by Y mode: Y is MSVP (Most Significant Value Part), X is LSVP
        double x_val;
        double y_val;
        int y_pos;
        int x_pos;

        y_val = modf(value, &x_val);
        y_val *= info->y_square_scale;

        // unsigned int plot_bidir: 1;   // plot zero (= 'offset_for_data') at middle Y position
        //
        // This one is already done in the precalcs
        y_val += info->y_middle_offset;

        y_pos = (int)y_val;
        // allow for a little floating point calc error:
        CRM_ASSERT(y_pos >= -1);
        CRM_ASSERT(y_pos <= info->y_height);
        if (y_pos < 0)
        {
            y_pos = 0;
        }
        else if (y_pos >= info->y_height)
        {
            y_pos = info->y_height - 1;
        }
        CRM_ASSERT(y_pos >= 0);
        CRM_ASSERT(y_pos < info->y_height);


        x_val *= info->x_square_scale;

        // unsigned int plot_bidir: 1;   // plot zero (= 'offset_for_data') at middle Y position (or is it middle X? it is for square abs pictures.)
        //
        // This one is already done in the precalcs
        x_val += info->x_middle_offset;

        x_pos = (int)x_val;
        // allow for a little floating point calc error:
        CRM_ASSERT(x_pos >= -1);
        CRM_ASSERT(x_pos <= info->x_width);
        if (x_pos < 0)
        {
            x_pos = 0;
        }
        else if (x_pos >= info->x_width)
        {
            x_pos = info->x_width - 1;
        }
        CRM_ASSERT(x_pos >= 0);
        CRM_ASSERT(x_pos < info->x_width);

        // plot value in additive mode.
        y_pos += x_pos * info->y_height;
        CRM_ASSERT(y_pos >= 0);
        CRM_ASSERT(y_pos < info->counterspace_size);
        info->counters[y_pos]++;
    }
}




int calculate_BMP_image_size(int x_width, int y_height)
{
    return sizeof(BmpFileStructure) + (x_width * y_height) * sizeof(((BmpFileStructure *)0)->bitmap[0]) - sizeof(((BmpFileStructure *)0)->bitmap);
}

//
// return number of bytes written to 'dst'; 0 on error.
//
int create_BMP_file_header(BmpFileStructure *dst, int bufsize, int x_width, int y_height)
{
    if (!dst || bufsize < sizeof(*dst) || x_width <= 0 || y_height <= 0)
        return 0;

    memset(dst, 0, sizeof(*dst));
    dst->header.type_id = n16_to_le(BMP_FILE_ID);
    dst->header.bitmap_data_offset = n32_to_le((int)(((char *)&dst->bitmap[0]) - ((char *)dst)));
    dst->header.file_size = n32_to_le(calculate_BMP_image_size(x_width, y_height));


    dst->info.header_size = n32_to_le(sizeof(dst->info));
    CRM_ASSERT(sizeof(dst->info) == 40);
    dst->info.image_width = n32_to_le(x_width);
    dst->info.image_height = n32_to_le(y_height);
    dst->info.color_planes = n16_to_le(1);
    dst->info.bits_per_pixel = n16_to_le(32);
    dst->info.compression_method = n32_to_le(0);
    dst->info.size_of_pixel_data = n32_to_le(x_width * y_height * sizeof(dst->bitmap[0]));
    dst->info.horizontal_resolution = n32_to_le((int)(96.0 * 1000 / 25.4)); // pixels per meter
    dst->info.vertical_resolution = n32_to_le((int)(96.0 * 1000 / 25.4));   // pixels per meter
    dst->info.number_of_colors = n32_to_le(0);                              // number of colors in palette; 0: true color
    dst->info.number_of_important_colors = n32_to_le(0);                    // should be 0

    return (int)(((char *)&dst->bitmap[0]) - ((char *)dst));
}


//
// pixels are 'ranged' to the range '0..1', i.e. these are a 'perunage' (part-per-one)
// of color brightness.
//
// Using this standard allows us to upgrade to HDRI imagery when we like/want, without the
// need to change the scaling/ranging code driving this thing...
//
// The same applies to 'x' and 'y': these are assumed to be 'ranged' to '0..1' representing 'full range'.
//
// Return 0 on success; negative on error, positive when the data was discarded ('clipped')
int plot_BMP_pixel(BmpFileStructure *dst, CRM_ANALYSIS_INFOBLOCK *info, double x, double y, RGBpixel *pixel)
{
    BmpPixel *px;
    int xpos;
    int ypos;
    int color;

    CRM_ASSERT(dst);
    CRM_ASSERT(info);

#define CLIP_CHECK_TOLERANCE  2.0

    x += info->x_middle_offset;
    x *= info->x_display_scale;
    if (x < -CLIP_CHECK_TOLERANCE || x >= info->x_display_size + CLIP_CHECK_TOLERANCE)
        return 1; // clipped due to x outside range

    xpos = (int)x;
    if (xpos <  0 || x >= info->x_width)
        return 1; // clipped due to x outside range

    y += info->y_middle_offset;
    y *= info->y_display_scale;
    if (y < -CLIP_CHECK_TOLERANCE || y >= info->y_display_size + CLIP_CHECK_TOLERANCE)
        return 1; // clipped due to x outside range

    ypos = (int)x;
    if (ypos <  0 || y >= info->y_height)
        return 1; // clipped due to x outside range

    // now we are sure to have legal x+y integer indexes into the bitmap
    //
    // calculate the colors next:
    px = &dst->bitmap[xpos + ypos * info->x_width];

    color = (int)(pixel->alpha * 256);
    if (color < 0)
        color = 0;
    else if (color > 255)
        color = 255;
    px->alpha = color;

    color = (int)(pixel->red * 256);
    if (color < 0)
        color = 0;
    else if (color > 255)
        color = 255;
    px->red = color;

    color = (int)(pixel->blue * 256);
    if (color < 0)
        color = 0;
    else if (color > 255)
        color = 255;
    px->blue = color;

    color = (int)(pixel->green * 256);
    if (color < 0)
        color = 0;
    else if (color > 255)
        color = 255;
    px->green = color;

    return 0;
}







/*
 * Device-independent bitmaps and BMP file format
 *
 * Microsoft has defined a particular representation of color bitmaps of different color depths, as an aid to exchanging bitmaps between devices and applications with a variety of internal representations. They called these device-independent bitmaps or DIBs, and the file format for them is called DIB file format or BMP file format. According to Microsoft support:[2]
 *
 * Since BMP/DIB format was created for the Intel X86 architecture, all integers are stored in little-endian format.
 *
 *  A device-independent bitmap (DIB) is a format used to define device-independent bitmaps in various color resolutions. The main purpose of DIBs is to allow bitmaps to be moved from one device to another (hence, the device-independent part of the name). A DIB is an external format, in contrast to a device-dependent bitmap, which appears in the system as a bitmap object (created by an application...). A DIB is normally transported in metafiles (usually using the StretchDIBits() function), BMP files, and the Clipboard (CF_DIB data format).
 *
 * A typical BMP file usually contains the following blocks of data:
 * BMP Header   Stores general information about the BMP file.
 * Bitmap Information (DIB header)      Stores detailed information about the bitmap image.
 * Color Palette        Stores the definition of the colors being used for indexed color bitmaps.
 * Bitmap Data  Stores the actual image, pixel by pixel.
 *
 * The following sections discuss the data stored in the BMP file or DIB in details. This is the standard BMP file format.[2] Some bitmap images may be stored using a slightly different format, depending on the application that creates it. Also, not all fields are used; a value of 0 will be found in these unused fields.
 *
 * [edit] DIBs in memory
 *
 * A BMP file is designed to have its contents loaded into memory as a DIB, an important component of the Windows GDI API. The BMP file format and the DIB data structure are identical except for the 14-uint8_t BMP header.
 *
 * [edit] BMP file header
 *
 * This block of uint8_ts is added before the DIB format used internally by GDI and serves for identification. A typical application will read this block first to ensure that the file is actually a BMP file and that it is not damaged. Note that while all of the other integer values are stored in little-endian format (i.e. least-significant uint8_t first), the magic number is not. Hence the first uint8_t in a BMP file is 'B', and the second uint8_t is 'M'.
 * Offset#      Size    Purpose
 * 0    2       the magic number used to identify the BMP file: 0x42 0x4D (ASCII code points for B and M)
 * 2    4       the size of the BMP file in uint8_ts
 * 6    2       reserved; actual value depends on the application that creates the image
 * 8    2       reserved; actual value depends on the application that creates the image
 * 10   4       the offset, i.e. starting address, of the uint8_t where the bitmap data can be found.
 *
 * [edit] Bitmap information (DIB header)
 *
 * This block of uint8_ts tells the application detailed information about the image, which will be used to display the image on the screen. The block also matches the header used internally by Windows and OS/2 and has several different variants. All of them contain a uint32_t field, specifying their size, so that an application can easily determine which header is used in the image. The reason that there are different headers is that Microsoft extended the DIB format several times. The new extended headers can be used with some GDI functions instead of the older ones, providing more functionality. Also the GDI supports a function for loading bitmap files, and it is likely that a Windows application uses it. One consequence of this is that the bitmap formats a program supports, match the formats supported by the Windows version it is running on. See the table below for more information.
 * Size         Header  Identified by   Supported by the GDI of
 * 40   Windows V3      BITMAPINFOHEADER        all Windows versions since Windows 3.0
 * 12   OS/2 V1         BITMAPCOREHEADER        OS/2 and also all Windows versions since Windows 3.0
 * 64   OS/2 V2
 * 108  Windows V4      BITMAPV4HEADER  all Windows versions since Windows 95/NT4
 * 124  Windows V5      BITMAPV5HEADER  Windows 98/2000 and newer
 *
 * For compatibility reasons, most applications use the older DIB headers for saving files. With OS/2 being obsolete, for now the only common format is the V3 header. See next table for its description. All values are stored as unsigned integers, unless explicitly noted.
 * Offset #     Size    Purpose
 * 14   4       the size of this header (40 uint8_ts)
 * 18   4       the bitmap width in pixels (signed integer).
 * 22   4       the bitmap height in pixels (signed integer).
 * 26   2       the number of color planes being used. Must be set to 1.
 * 28   2       the number of bits per pixel, which is the color depth of the image. Typical values are 1, 4, 8, 16, 24 and 32.
 * 30   4       the compression method being used. See the next table for a list of possible values.
 * 34   4       the image size. This is the size of the raw bitmap data (see below), and should not be confused with the file size.
 * 38   4       the horizontal resolution of the image. (pixel per meter, signed integer)
 * 42   4       the vertical resolution of the image. (pixel per meter, signed integer)
 * 46   4       the number of colors in the color palette, or 0 to default to 2n.
 * 50   4       the number of important colors used, or 0 when every color is important; generally ignored.
 *
 * The compression method field (uint8_ts #30-33) can have the following values:
 * Value        Identified by   Compression method      Comments
 * 0    BI_RGB  none    Most common
 * 1    BI_RLE8         RLE 8-bit/pixel         Can be used only with 8-bit/pixel bitmaps
 * 2    BI_RLE4         RLE 4-bit/pixel         Can be used only with 4-bit/pixel bitmaps
 * 3    BI_BITFIELDS    Bit field       Can be used only with 16 and 32-bit/pixel bitmaps.
 * 4    BI_JPEG         JPEG    The bitmap contains a JPEG image
 * 5    BI_PNG  PNG     The bitmap contains a PNG image
 *
 * Note: The image size field can be 0 for BI_RGB bitmaps.
 *
 * The OS/2 V1 header is also popular:
 * Offset       Size    Purpose
 * 14   4       the size of this header (12 uint8_ts)
 * 18   2       the bitmap width in pixels.
 * 20   2       the bitmap height in pixels.
 * 22   2       the number of color planes; 1 is the only legal value
 * 24   2       the number of bits per pixel. Typical values are 1, 4, 8 and 24.
 *
 * Note: OS/2 V1 bitmaps cannot be compressed and cannot be 16 or 32 bits/pixel. All values in the OS/2 V1 header are unsigned integers.
 *
 * A 32-bit version of DIB with integrated alpha channel has been introduced with Windows XP and is used within its logon and theme system; it has yet to gain wide support in image editing software, but has been supported in Adobe Photoshop since version 7 and Adobe Flash since version MX 2004 (then known as Macromedia Flash).
 *
 * [edit] Color palette
 *
 * The palette, or color table, is a block of uint8_ts defining the colors available for use in an indexed-color image. Each pixel in the bitmap is described by a few bits (1, 4, or 8) representing an index into this color table. That is, the purpose of the color palette in indexed-color bitmaps is to tell the application the actual color that each of these index values corresponds to.
 *
 * A DIB always uses the RGB color model. In this model, a color is terms of different intensities (from 0 to 255) of the additive primary colors red (R), green (G), and blue (B). A color is thus defined using the 3 values for R, G and B (though stored in backwards order in each palette entry).
 *
 * The number of entries in the palette is either 2n or a smaller number specified in the header (in the OS/2 V1 format, only the full-size palette is supported).[2][3] Each entry contains four uint8_ts, except in the case of the OS/2 V1 versions, in which case there are only three uint8_ts per entry.[3] The first (and only for OS/2 V1) three uint8_ts store the values for blue, green, and red, respectively,[2] while the last one is unused and is filled with 0 by most applications.
 *
 * As mentioned above, the color palette is not used when the bitmap is 16-bit or higher; there are no palette uint8_ts in those cases.
 *
 * [edit] Bitmap data
 *
 * This block of uint8_ts describes the image, pixel by pixel. Pixels are stored "upside-down" with respect to normal image raster scan order, starting in the lower left corner, going from left to right, and then row by row from the bottom to the top of the image.[2]
 *
 * Uncompressed Windows bitmaps can also be stored from the top row to the bottom, if the image height value is negative. If the number of uint8_ts matching a row (scanline) in the image is not divisible by 4, the line is padded with one to three additional uint8_ts of unspecified value so that the next row will start on a multiple of 4 uint8_t location in memory or in the file. Following these rules there are several ways to store the pixel data depending on the color depth and the compression type of the bitmap.
 *
 * [edit] Usage of BMP format
 *
 * The simplicity of the BMP file format, and its widespread familiarity in Windows and elsewhere, as well as the fact that this format is relatively well documented and free of patents, makes it a very common format that image processing programs from many operating systems can read and write.
 *
 * While most BMP files have a relatively large file size due to lack of any compression, many BMP files can be considerably compressed with lossless data compression algorithms such as ZIP because they contain redundant data.
 *
 * [edit]
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 * bmp contents
 *
 * the following table contains a description of the contents of the bmp file. for every field, the file offset, the length and the contents will be given. for a more detailed discussion, see the following chapters.
 *
 * offset
 *
 *
 * field
 *
 *
 * size
 *
 *
 * contents
 *
 * 0000h
 *
 *
 * identifier
 *
 *
 * 2 uint8_ts
 *
 *
 * the characters identifying the bitmap. the following entries are possible:
 *
 * ?m? - windows 3.1x, 95, nt, ?
 *
 * ?a? - os/2 bitmap array
 *
 * ?i? - os/2 color icon
 *
 * ?p? - os/2 color pointer
 *
 * ?c? - os/2 icon
 *
 * ?t? - os/2 pointer
 *
 * 0002h
 *
 *
 * file size
 *
 *
 * 1 uint32_t
 *
 *
 * complete file size in uint8_ts.
 *
 * 0006h
 *
 *
 * reserved
 *
 *
 * 1 uint32_t
 *
 *
 * reserved for later use.
 *
 * 000ah
 *
 *
 * bitmap data offset
 *
 *
 * 1 uint32_t
 *
 *
 * offset from beginning of file to the beginning of the bitmap data.
 *
 * 000eh
 *
 *
 * bitmap header size
 *
 *
 * 1 uint32_t
 *
 *
 * length of the bitmap info header used to describe the bitmap colors, compression, ? the following sizes are possible:
 *
 * 28h - windows 3.1x, 95, nt, ?
 *
 * 0ch - os/2 1.x
 *
 * f0h - os/2 2.x
 *
 * 0012h
 *
 *
 * width
 *
 *
 * 1 uint32_t
 *
 *
 * horizontal width of bitmap in pixels.
 *
 * 0016h
 *
 *
 * height
 *
 *
 * 1 uint32_t
 *
 *
 * vertical height of bitmap in pixels.
 *
 * 001ah
 *
 *
 * planes
 *
 *
 * 1 word
 *
 *
 * number of planes in this bitmap.
 *
 * 001ch
 *
 *
 * bits per pixel
 *
 *
 * 1 word
 *
 *
 * bits per pixel used to store palette entry information. this also identifies in an indirect way the number of possible colors. possible values are:
 *
 * 1 - monochrome bitmap
 *
 * 4 - 16 color bitmap
 *
 * 8 - 256 color bitmap
 *
 * 16 - 16bit (high color) bitmap
 *
 * 24 - 24bit (true color) bitmap
 *
 * 32 - 32bit (true color) bitmap
 *
 * 001eh
 *
 *
 * compression
 *
 *
 * 1 uint32_t
 *
 *
 * compression specifications. the following values are possible:
 *
 * 0 - none (also identified by bi_rgb)
 *
 * 1 - rle 8-bit / pixel (also identified by bi_rle4)
 *
 * 2 - rle 4-bit / pixel (also identified by bi_rle8)
 *
 * 3 - bitfields (also identified by bi_bitfields)
 *
 * 0022h
 *
 *
 * bitmap data size
 *
 *
 * 1 uint32_t
 *
 *
 * size of the bitmap data in uint8_ts. this number must be rounded to the next 4 uint8_t boundary.
 *
 * 0026h
 *
 *
 * hresolution
 *
 *
 * 1 uint32_t
 *
 *
 * horizontal resolution expressed in pixel per meter.
 *
 * 002ah
 *
 *
 * vresolution
 *
 *
 * 1 uint32_t
 *
 *
 * vertical resolution expressed in pixels per meter.
 *
 * 002eh
 *
 *
 * colors
 *
 *
 * 1 uint32_t
 *
 *
 * number of colors used by this bitmap. for a 8-bit / pixel bitmap this will be 100h or 256.
 *
 * 0032h
 *
 *
 * important colors
 *
 *
 * 1 uint32_t
 *
 *
 * number of important colors. this number will be equal to the number of colors when every color is important.
 *
 * 0036h
 *
 *
 * palette
 *
 *
 * n * 4 uint8_t
 *
 *
 * the palette specification. for every entry in the palette four uint8_ts are used to describe the rgb values of the color in the following way:
 *
 * 1 uint8_t for blue component
 * Transfer interrupted!
 * p>
 *
 * 1 uint8_t for red component
 *
 * 1 uint8_t filler which is set to 0 (zero)
 *
 * 0436h
 *
 *
 * bitmap data
 *
 *
 * x uint8_ts
 *
 *
 * depending on the compression specifications, this field contains all the bitmap data uint8_ts which represent indices in the color palette.
 *
 * ?
 *
 * note: the following sizes were used in the specification above:
 *
 * size
 *
 *
 # uint8_ts
 *
 *
 * sign
 *
 * char
 *
 *
 * 1
 *
 *
 * signed
 *
 * word
 *
 *
 * 2
 *
 *
 * unsigned
 *
 * uint32_t
 *
 *
 * 4
 *
 *
 * unsigned
 *
 * field details
 *
 * some of the fields require some more information. the following chapters will try to provide this information:
 *
 * height field
 *
 * the height field identifies the height of the bitmap in pixels. in other words, it describes the number of scan lines of the bitmap. if this field is negative, indicating a top-down dib, the compression field must be either bi_rgb or bi_bitfields. top-down dibs cannot be compressed.
 *
 * bits per pixel field
 *
 * the bits per pixel (bbp) field of the bitmap file determines the number of bits that define each pixel and the maximum number of colors in the bitmap.
 *
 * when this field is equal to 1.
 *
 * the bitmap is monochrome, and the palette contains two entries. each bit in the bitmap array represents a pixel. if the bit is clear, the pixel is displayed with the color of the first entry in the palette; if the bit is set, the pixel has the color of the second entry in the table.
 *
 * ?
 *
 * when this field is equal to 4.
 *
 * the bitmap has a maximum of 16 colors, and the palette contains up to 16 entries. each pixel in the bitmap is represented by a 4-bit index into the palette. for example, if the first uint8_t in the bitmap is 1fh, the uint8_t represents two pixels. the first pixel contains the color in the second palette entry, and the second pixel contains the color in the sixteenth palette entry.
 *
 * ?
 *
 * when this field is equal to 8.
 *
 * the bitmap has a maximum of 256 colors, and the palette contains up to 256 entries. in this case, each uint8_t in the array represents a single pixel.
 *
 * ?
 *
 * when this field is equal to 16.
 *
 * the bitmap has a maximum of 2^16 colors. if the compression field of the bitmap file is set to bi_rgb, the palette field does not contain any entries. each word in the bitmap array represents a single pixel. the relative intensities of red, green, and blue are represented with 5 bits for each color component. the value for blue is in the least significant 5 bits, followed by 5 bits each for green and red, respectively. the most significant bit is not used.
 *
 * if the compression field of the bitmap file is set to bi_bitfields, the palette field contains three uint32_t color masks that specify the red, green, and blue components, respectively, of each pixel. each word in the bitmap array represents a single pixel.
 *
 * windows nt specific: when the compression field is set to bi_bitfields, bits set in each uint32_t mask must be contiguous and should not overlap the bits of another mask. all the bits in the pixel do not have to be used.
 *
 * windows 95 specific: when the compression field is set to bi_bitfields, windows 95 supports only the following 16bpp color masks: a 5-5-5 16-bit image, where the blue mask is 0x001f, the green mask is 0x03e0, and the red mask is 0x7c00; and a 5-6-5 16-bit image, where the blue mask is 0x001f, the green mask is 0x07e0, and the red mask is 0xf800.
 *
 * ?
 *
 * when this field is equal to 24.
 *
 * the bitmap has a maximum of 2^24 colors, and the palette field does not contain any entries. each 3-uint8_t triplet in the bitmap array represents the relative intensities of blue, green, and red, respectively, for a pixel.
 *
 * ?
 *
 * when this field is equal to 32.
 *
 * the bitmap has a maximum of 2^32 colors. if the compression field of the bitmap is set to bi_rgb, the palette field does not contain any entries. each uint32_t in the bitmap array represents the relative intensities of blue, green, and red, respectively, for a pixel. the high uint8_t in each uint32_t is not used.
 *
 * if the compression field of the bitmap is set to bi_bitfields, the palette field contains three uint32_t color masks that specify the red, green, and blue components, respectively, of each pixel. each uint32_t in the bitmap array represents a single pixel.
 *
 * windows nt specific: when the compression field is set to bi_bitfields, bits set in each uint32_t mask must be contiguous and should not overlap the bits of another mask. all the bits in the pixel do not have to be used.
 *
 * windows 95 specific: when the compression field is set to bi_bitfields, windows 95 supports only the following 32bpp color mask: the blue mask is 0x000000ff, the green mask is 0x0000ff00, and the red mask is 0x00ff0000.
 *
 * compression field
 *
 * the compression field specifies the way the bitmap data is stored in the file. this information together with the bits per pixel (bpp) field identifies the compression algorithm to follow.
 *
 * the following values are possible in this field:
 *
 * value
 *
 *
 * meaning
 *
 * bi_rgb
 *
 *
 * an uncompressed format.
 * lign="justify">bi_rle4
 *
 * an rle format for bitmaps with 4 bits per pixel. the compression format is a two-uint8_t format consisting of a count uint8_t followed by two word-length color indices. for more information, see the following remarks section.
 *
 * bi_rle8
 *
 *
 * a run-length encoded (rle) format for bitmaps with 8 bits per pixel. the compression format is a two-uint8_t format consisting of a count uint8_t followed by a uint8_t containing a color index. for more information, see the following remarks section.
 *
 * bi_bitfields
 *
 *
 * specifies that the bitmap is not compressed and that the color table consists of three double word color masks that specify the red, green, and blue components, respectively, of each pixel. this is valid when used with 16- and 32- bits-per-pixel bitmaps.
 *
 * when the compression field is bi_rle8, the bitmap is compressed by using a run-length encoding (rle) format for an 8-bit bitmap. this format can be compressed in encoded or absolute modes. both modes can occur anywhere in the same bitmap.
 *
 * encoded mode consists of two uint8_ts:
 *
 * the first uint8_t specifies the number of consecutive pixels to be drawn using the color index contained in the second uint8_t. in addition, the first uint8_t of the pair can be set to zero to indicate an escape that denotes an end of line, end of bitmap, or delta. the interpretation of the escape depends on the value of the second uint8_t of the pair, which can be one of the following:
 *
 * 0
 *
 *
 * end of line.
 *
 * 1
 *
 *
 * end of bitmap.
 *
 * 2
 *
 *
 * delta. the two uint8_ts following the escape contain unsigned values indicating the horizontal and vertical offsets of the next pixel from the current position.
 *
 * absolute mode.
 *
 * the first uint8_t is zero and the second uint8_t is a value in the range 03h through ffh. the second uint8_t represents the number of uint8_ts that follow, each of which contains the color index of a single pixel. when the second uint8_t is 2 or less, the escape has the same meaning as in encoded mode. in absolute mode, each run must be aligned on a word boundary.
 *
 * ?
 *
 * ?
 *
 * the following example shows the hexadecimal values of an 8-bit compressed bitmap.
 *
 * 03 04 05 06 00 03 45 56 67 00 02 78 00 02 05 01 02 78 00 00 09 1e 00 01
 *
 * this bitmap would expand as follows (two-digit values represent a color index for a single pixel):
 *
 * 04 04 04
 *
 * 06 06 06 06 06
 *
 * 45 56 67
 *
 * 78 78
 *
 * move current position 5 right and 1 down
 *
 * 78 78
 *
 * end of line
 *
 * 1e 1e 1e 1e 1e 1e 1e 1e 1e
 *
 * end of rle bitmap
 *
 * ?
 *
 * when the compression field is bi_rle4, the bitmap is compressed by using a run-length encoding format for a 4-bit bitmap, which also uses encoded and absolute modes:
 *
 * in encoded mode.
 *
 * the first uint8_t of the pair contains the number of pixels to be drawn using the color indices in the second uint8_t. the second uint8_t contains two color indices, one in its high-order four bits and one in its low-order four bits. the first of the pixels is drawn using the color specified by the high-order four bits, the second is drawn using the color in the low-order four bits, the third is drawn using the color in the high-order four bits, and so on, until all the pixels specified by the first uint8_t have been drawn.
 *
 * in absolute mode.
 *
 * the first uint8_t is zero, the second uint8_t contains the number of color indices that follow, and subsequent uint8_ts contain color indices in their high- and low-order four bits, one color index for each pixel. in absolute mode, each run must be aligned on a word boundary.
 *
 * the end-of-line, end-of-bitmap, and delta escapes described for bi_rle8 also apply to bi_rle4 compression.
 *
 * ?
 *
 * the following example shows the hexadecimal values of a 4-bit compressed bitmap.
 *
 * 03 04 05 06 00 06 45 56 67 00 04 78 00 02 05 01 04 78 00 00 09 1e 00 01
 *
 * this bitmap would expand as follows (single-digit values represent a color index for a single pixel):
 *
 * 0 4 0
 *
 * 0 6 0 6 0
 *
 * 4 5 5 6 6 7
 *
 * 7 8 7 8
 *
 * move current position 5 right and 1 down
 *
 * 7 8 7 8
 *
 * end of line
 *
 * 1 e 1 e 1 e 1 e 1
 *
 * end of rle bitmap
 *
 * colors field
 *
 * the colors field specifies the number of color indices in the color table that are actually used by the bitmap. if this value is zero, the bitmap uses the maximum number of colors corresponding to the value of the bbp field for the compression mode specified by the compression field.
 *
 * if the colors field is nonzero and the bbp field less than 16, the colors field specifies the actual number of colors the graphics engine or device driver accesses.
 *
 * if the bbp field is 16 or greater, then colors field specifies the size of the color table used to optimize performance of windows color palettes.
 *
 * if bbp equals 16 or 32, the optimal color palette starts immediately following the three double word masks.
 *
 * if the bitmap is a packed bitmap (a bitmap in which the bitmap array immediately follows the bitmap header and which is referenced by a single pointer), the colors field must be either 0 or the actual size of the color table.
 *
 * important colors field
 *
 * the important colors field specifies the number of color indices that are considered important for displaying the bitmap. if this value is zero, all colors are important.
 */

