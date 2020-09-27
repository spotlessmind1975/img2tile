# img2tile
Utility to convert images into (a set of) tile(s)

## USAGE

<pre>img2tile.exe [options]</pre>

By omitting the options, you get online help.

## OPTIONS (mandatory)

`-i <filename>` input filename

With this option you can specify the image file(s) to be processed (up to 256 files). When multiple files are given, each image will be read and put at the end of the set of tiles. The supported input formats are as follows:

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

`-b <number>`   set the bank number (used only with `-g`)

It is possible to indicate on which bank of characters these tiles will be loaded. The number entered will be used in the generation of C source codes. In particular, the number put here will be added to the symbols `_TILES_` / `_TILE_name` (as: `_TILES<number>_` / `_TILE<number>_name`). For backwards compatibility, no number will be added for bank number zero. So `-b 0` is equal to not specifying the `-b` option.

`-g <filename>` generate C header of tile offset

If this option is given, a C header file will be created. In this file will be defined some constants, that are useful to access to each tile generated:
  * `_TILES_` : used to isolate the C header file, to avoid multiple include file;
  * `TILE_START` : the index of the first tile of the tile set (normally: 0);
  * `TILE_name` : the index of the first tile for the image `name` (the name is generated based on the original file name); 
  * `TILE_name_WIDTH` : the width of the image `name`, in term of tiles;
  * `TILE_name_HEIGHT` : the height of the image `name`, in term of tiles;
  * `TILE_COUNT` : the number of tiles present into the generated file.

`-l <lum>`      threshold luminance

It is possible to indicate the luminance threshold, above which the source pixel is considered as "on" and below which the pixel is considered "off". A value of zero implies that all "on" pixels will be drawn. Conversely, a too high value of this parameter will result in a completely "off" image.

`-m`            enable multicolor supprot

It is possible to indicate if the tiles are to be created in "multicolor" mode. In this mode, the color index of each pixel is decided by the combination of two pixels and not just one. This implies that the output resolution will not be 8x8 pixels but 4x8 pixels, and so must be the input resolution. In other words: the width must be a multiple of 4 pixels (and not 8 pixels) and, moreover, no more than four different colors must be used for drawing. The assignment of the indices to the colors is carried out sequentially, from left to right and from top to bottom.

`-q`            quiet execution

This option disables any type of output, making the program suitable for running in a batch or makefile context.

`-R`            reverse "on"/"off" pixel

This option will invert the meaning of luminance: when a pixel is "on", the pixel on tile will be drawn as "off", and vice versa.

`-v`            make execution verbose

Activates the display of all essential information, as well as an ASCII representation of the processed image.