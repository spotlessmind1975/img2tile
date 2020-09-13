/****************************************************************************
 * img2tile - Utility to convert images into (a set of) tiles               *
 *                                                                          *
 * Copyright (c) 2020 by Marco Spedaletti. Licensed under CC-BY-NC-SA       *
 *--------------------------------------------------------------------------*
 * INCLUDE FILE                                                             *
 ****************************************************************************/

#ifndef _IMG2TILE_H_
#define _IMG2TILE_H_

    /************************************************************************
     * ------ DATA TYPES
     ************************************************************************/

    // This structure stores the color components (red, blue and green) of a 
    // pixel, 8 bits wide. This structure is used both to represent the 
    // retrocomputer palette and to process input data from image files.
    typedef struct {
        int red;
        int green;
        int blue;
    } RGB;

    // This structure maintains the program's options, and used by the process 
    // of converting from image files to midres.

    typedef struct {

        int width;

        int height;

        int depth;

        int reverse;

        int width_tiles;

        int height_tiles;

        int luminance_threshold;

        int bank;

    } Configuration;

    // This structure maintain the result of conversion operation.

    typedef struct {

        mr_tile     tiles_count;
        mr_mixel*   tiles;

    } Output;

#endif
