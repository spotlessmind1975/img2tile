# img2tile
Utility to convert images into (a set of) tile(s)

## USAGE

<pre>img2tile.exe [options]</pre>

By omitting the options, you get online help.

## OPTIONS (mandatory)

`-i <filename>` input filename

With this option you can specify the image file to be processed. The supported input formats are as follows:

 * JPEG (12 bpc not supported)
 * PNG 1...16 bpc
 * TGA
 * BMP >1bpp, non-RLE
 * PSD (composited view only, no extra channels, 8/16  bpc)
 * GIF
 * HDR (rgbE)
 * PIC (Softimage)
 * PNM (PPM / PGM binary)

`-o <filename>` output filename

With this option you can indicate the name of the file where the tile(s) will be written. By convention, the following extensions should be used:
 * .bin - for charset definitition

## OPTIONS

`-l <lum>`      threshold luminance

It is possible to indicate the luminance threshold, above which the source pixel is considered as "on" and below which the pixel is considered "off". A value of zero implies that all "on" pixels will be drawn. Conversely, a too high value of this parameter will result in a completely "off" image.

`-q`            quiet execution

This option disables any type of output, making the program suitable for running in a batch or makefile context.

`-R`            reverse "on"/"off" pixel

This option will invert the meaning of luminance: when a pixel is "on", the pixel on tile will be drawn as "off", and vice versa.

`-v`            make execution verbose

Activates the display of all essential information, as well as an ASCII representation of the processed image.