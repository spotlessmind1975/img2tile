/****************************************************************************
 * img2tile - Utility to convert images into (a set of                *
 *                                                                          *
 * Copyright (c) 2020 by Marco Spedaletti. Licensed under CC-BY-NC-SA       *
 ****************************************************************************/

// This directive is used to deactivate the safety warning for the use 
// of fopen under Microsoft Visual Studio.
#pragma warning(disable : 4996)

/****************************************************************************
 ** INCLUDE SECTION
 ****************************************************************************/

#include "..\midres\src\midres.h"
#include "img2tile.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <ctype.h>

/****************************************************************************
 ** RESIDENT VARIABLES SECTION
 ****************************************************************************/

// Error levels.

#define ERL_WRONG_OPTIONS               1
#define ERL_MISSING_INPUT_FILENAME      2
#define ERL_MISSING_OUTPUT_FILENAME     3
#define ERL_CANNOT_OPEN_INPUT           5
#define ERL_CANNOT_OPEN_OUTPUT          6
#define ERL_CANNOT_CONVERT_WIDTH        7
#define ERL_CANNOT_CONVERT_HEIGHT       8

// These are the default values for running the program.

Configuration configuration = {
    8,  /* width */
    8,  /* height */
    3,  /* depth */
    0,  /* reverse */
    1,  /* width_tiles */
    1,  /* height_tiles */
    1   /* luminance_threshold */
};

// Pointer to the name of the file with the image to be processed.

char* filename_in = NULL;

// Pointer to the name of the file with the tile data.

char* filename_out = NULL;

// Verbose?

int verbose = 0;

/****************************************************************************
 ** RESIDENT FUNCTIONS SECTION
 ****************************************************************************/

// Extract the basename part of a complete file path

char* basename(char* _path) {
    char* base = strrchr(_path, '/');
    if (base == NULL) {
        base = strrchr(_path, '\\');
    }
    return base ? base + 1 : _path;
}

// This function prints a short guide to the available options.

void usage_and_exit(int _level, int _argc, char* _argv[]) {

    printf("img2tile - Utility to convert images into (a set of) tile(s)\n");
    printf("Copyright(c) 2020 by Marco Spedaletti.\n");
    printf("Licensed under CC-BY-NC-SA\n\n");
    printf("Usage: %s [options]", _argv[0]);
    printf("\n");
    printf("options:\n");
    printf("\n");
    printf("[mandatory]\n");
    printf("\n");
    printf(" -i <filename> input filename\n");
    printf("                  supported formats: \n");
    printf("                    JPEG (12 bpc not supported)\n");
    printf("                    PNG 1...16 bpc\n");
    printf("                    TGA\n");
    printf("                    BMP >1bpp, non-RLE\n");
    printf("                    PSD (composited view only, no extra channels, 8/16  bpc)\n");
    printf("                    GIF\n");
    printf("                    HDR (rgbE)\n");
    printf("                    PIC (Softimage)\n");
    printf("                    PNM (PPM / PGM binary)\n");
    printf(" -o <filename> output filename\n");
    printf("\n");
    printf("[optional]\n");
    printf("\n");
    printf(" -l <lum>      threshold luminance\n");
    printf(" -R            reverse luminance threshold\n");
    printf(" ");

    exit(_level);

}

// This function allows to parse the options entered on the command line. 
// Options must start with a minus character ('-') and only the first letter 
// is considered.

void parse_options(int _argc, char* _argv[]) {

    // Used as index.
    int i;

    // We check for each option...
    for (i = 1; i < _argc; ++i) {

        // Parse it only if begins with '-'
        if (_argv[i][0] == '-') {

            switch (_argv[i][1]) {
                case 'i': // "-i <filename>"
                    filename_in = _argv[i + 1];
                    ++i;
                    break;
                case 'o': // "-o <filename>"
                    filename_out = _argv[i + 1];
                    ++i;
                    break;
                case 'l': // "-l <luminance>"
                    configuration.luminance_threshold = atoi(_argv[i + 1]);
                    ++i;
                    break;
                case 'R': // "-R"
                    configuration.reverse = 1;
                    break;
                case 'v': // "-v"
                    verbose = 1;
                    break;
                case 'q': // "-q"
                    verbose = 0;
                    break;
                default:
                    printf("Unknown option: %s", _argv[i]);
                    usage_and_exit(ERL_WRONG_OPTIONS, _argc, _argv);
            }

        }
    }

}

// This function calculates the luminance of a color. 
// By luminance we mean the modulus of the three-dimensional vector, drawn 
// in the space composed of the three components (red, green and blue).
// The returned value is normalized to the nearest 8-bit value.

int calculate_luminance(RGB _a) {

    // Extract the vector's components 
    // (each partecipate up to 1/3 of the luminance).
    double red = (double)_a.red / 3;
    double green = (double)_a.green / 3;
    double blue = (double)_a.blue / 3;

    // Calculate luminance using Pitagora's Theorem
    return (int)sqrt(pow(red, 2) + pow(green, 2) + pow(blue, 2));

}

// This function convert an image of (W,H) pixels in a set of (WT,HT) tiles.
// Tiles will be drawn in a "contiguous" way, i.e. each row of tiles will
// be drawn sequentially, and each column for each row the same. 

void convert_image_into_tiles(unsigned char *_source, Configuration * _configuration, Output * _output ) {

    // Position of the pixel in the original image
    int image_x, image_y;
    
    // Position of the pixel, in terms of tiles
    int tile_x, tile_y;
    
    // Position of the pixel, in terms of offset and bitmask
    int offset, bitmask;

    // Color of the pixel to convert
    RGB rgb;

    // Calculate the surface area, in terms of tiles
    _output->tiles_count = _configuration->width_tiles * _configuration->height_tiles;

    // Allocate enough memory
    _output->tiles = malloc(_output->tiles_count * 8);

    // Clear the tiles.
    memset(_output->tiles, 0, _output->tiles_count * 8);

    // Loop for all the source surface.
    for (image_y = 0; image_y < _configuration->height; ++image_y) {
        for (image_x = 0; image_x < _configuration->width; ++image_x) {

            // Take the color of the pixel
            rgb.red = *_source;
            rgb.green = *(_source + 1);
            rgb.blue = *(_source + 2);

            // Calculate the relative tile
            tile_y = (image_y >> 3);
            tile_x = (image_x >> 3);
            
            // Calculate the offset starting from the tile surface area
            // and the bit to set.
            offset = (tile_y * 8 * _configuration->width_tiles) + (tile_x * 8) + (image_y & 0x07);
            bitmask = 1 << ( 7 - (image_x & 0x7) );

            // If the pixes has enough luminance value, it must be 
            // considered as "on"; otherwise, it is "off".
            if (calculate_luminance(rgb) >= _configuration->luminance_threshold) {

                // Inversion of "on"-"off" meaning.
                if (_configuration->reverse) {
                    *(_output->tiles + offset) &= ~bitmask;
                    printf(" ");
                }
                else {
                    *(_output->tiles + offset) |= bitmask;
                    printf("*");
                }
            }
            else {

                // Inversion of "on"-"off" meaning.
                if (!_configuration->reverse) {
                    *(_output->tiles + offset) &= ~bitmask;
                    printf(" ");
                }
                else {
                    *(_output->tiles + offset) |= bitmask;
                    printf("*");
                }
            }

            _source += _configuration->depth;

        }
        printf("\n");

    }

}

// Main function

int main(int _argc, char *_argv[]) {

    parse_options(_argc, _argv);

    if (filename_in == NULL) {
        printf("Missing input filename.\n");
        usage_and_exit(ERL_MISSING_INPUT_FILENAME, _argc, _argv);
    }

    if (filename_out == NULL) {
        printf("Missing output filename for luminance.\n");
        usage_and_exit(ERL_MISSING_OUTPUT_FILENAME, _argc, _argv);
    }

    if (verbose) {
        printf("Input image ................. %s\n", filename_in);
        printf("Output tile(s) .............. %s\n", filename_out);
    }

    unsigned char *source = stbi_load(filename_in, &configuration.width, &configuration.height, &configuration.depth, 0);

    if (source == NULL) {
        printf("Unable to open file %s\n", filename_in);
        usage_and_exit(ERL_CANNOT_OPEN_INPUT, _argc, _argv);
    }

    configuration.width_tiles = configuration.width >> 3;

    if ( (configuration.width & 0x08 ) != 0 ) {
        printf("Cannot convert images with width not multiple of 8 pixels.\n");
        usage_and_exit(ERL_CANNOT_CONVERT_WIDTH, _argc, _argv);
    }

    configuration.height_tiles = configuration.height >> 3;

    if ((configuration.height & 0x08) != 0) {
        printf("Cannot convert images with height not multiple of 8 pixels.\n");
        usage_and_exit(ERL_CANNOT_CONVERT_HEIGHT, _argc, _argv);
    }

    if (verbose) {
        printf(" %s: (%dx%d, %d bpp) -> (%dx%d, 1 bpp)\n", filename_in, configuration.width, configuration.height, configuration.depth, configuration.width_tiles, configuration.height_tiles );
    }

    Output result;
    convert_image_into_tiles(source, &configuration, &result);
    stbi_image_free(source);

    FILE *handle = fopen(filename_out, "w+b");
    if (handle == NULL) {
        printf("Unable to open file %s\n", filename_out);
        usage_and_exit(ERL_CANNOT_OPEN_OUTPUT, _argc, _argv);
    }

    fwrite(result.tiles, 8, result.tiles_count, handle);
    fclose(handle);

}