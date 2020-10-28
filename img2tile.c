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

// Maximum number of images

#define MAX_FILENAMES                   256

// Error levels.

#define ERL_WRONG_OPTIONS               1
#define ERL_MISSING_INPUT_FILENAME      2
#define ERL_MISSING_OUTPUT_FILENAME     3
#define ERL_CANNOT_OPEN_INPUT           5
#define ERL_CANNOT_OPEN_OUTPUT          6
#define ERL_CANNOT_CONVERT_WIDTH        7
#define ERL_CANNOT_CONVERT_HEIGHT       8
#define ERL_CANNOT_OPEN_HEADER          9
#define ERL_CANNOT_CONVERT_COLORS       10

// This is the default palette for supported retrocomputers.
// Data taken from: 
// - https://lospec.com/palette-list/commodore64
// - https://retroshowcase.gr/index.php?p=palette (color pick)

NamedRGB COLORS[] = {
    // C64 and VIC20 colors
    { "BLACK", { 0x00, 0x00, 0x00 } },
    { "WHITE", { 0xff, 0xff, 0xff } },
    { "RED", { 0x88, 0x00, 0x00 } },
    { "CYAN", { 0xaa, 0xff, 0xe6 } },
    { "VIOLET", { 0xcc, 0x44, 0xcc } },
    { "GREEN", { 0x00, 0xcc, 0x55 } },
    { "BLUE", { 0x00, 0x00, 0xaa } },
    { "YELLOW", { 0xee, 0xee, 0x77 } },
    { "ORANGE", { 0xa1, 0x68, 0x3c } },
    { "BROWN", { 0xdd, 0x88, 0x65 } },
    { "LIGHT_RED", { 0xff, 0x77, 0x77 } },
    { "DARK_GREY", { 0x33, 0x33, 0x33 } },
    { "GREY", { 0x77, 0x77, 0x77 } },
    { "LIGHT_GREEN", { 0xaa, 0xff, 0x66 } },
    { "LIGHT_BLUE", { 0x00, 0x88, 0xff } },
    { "LIGHT_GREY", { 0xbb, 0xbb, 0xbb } },
    { "PURPLE", { 0xbc, 0x52, 0xcc } },
    { "YELLOW_GREEN", { 0x61, 0x9e, 0x33 } },
    { "PINK", { 0xbc, 0x61, 0x80 } },
    { "BLUE_GREEN", { 0x43, 0x9e, 0x80 } },
    { "LIGHT_BLUE", { 0x43, 0x90, 0xcc } },
    { "DARK BLUE", { 0x9e, 0x61, 0xcc } },
    { "LIGHT_GREEN", { 0x00, 0xff, 0x2c } },
    { "MAGENTA", { 0xf9, 0x84, 0xe5 } },
    { "LAVENDER", { 0xe6, 0xe6, 0xfa } },
    { "GOLD", { 0xd4, 0xaf, 0x37 } },
    { "TAN", { 0xd2, 0xb4, 0x8c } },
    { "OLIVE_GREEN", { 0x55, 0x6b, 0x2f } },
    { "PEACH", { 0xff, 0xda, 0xb9 } }
};

// These are the default values for running the program.

Configuration configuration = {
    8,  /* width */
    8,  /* height */
    3,  /* depth */
    0,  /* reverse */
    1,  /* width_tiles */
    1,  /* height_tiles */
    1,  /* luminance_threshold */
    0,  /* bank number */
    0,  /* multicolor */
    -1  /* background */
};

// Pointer to the name of the file with the image to be processed.

char* filename_in[MAX_FILENAMES];

// Array with starting tile for each image

int starting_tile[MAX_FILENAMES];

// Array with widths (in tiles) for each image

int width_in_tiles[MAX_FILENAMES];

// Array with heights (in tiles) for each image

int height_in_tiles[MAX_FILENAMES];

// Count of images.

int filename_in_count = 0;

// Pointer to the name of the file with the tile data.

char* filename_out = NULL;

// Pointer to the name of the file with the C headers

char* filename_header = NULL;

// Verbose?

int verbose = 0;

// Debug?

int debug = 0;

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

    int i;

    printf("\n");
    printf("\n");
    printf("------------------------------------------------------------\n");
    printf("img2tile - Utility to convert images into (a set of) tile(s)\n");
    printf("------------------------------------------------------------\n");
    printf("Copyright(c) 2020 by Marco Spedaletti.\n");
    printf("Licensed under CC-BY-NC-SA\n\n");

    printf("Usage: %s [options]", _argv[0]);
    printf("\n");
    printf("options:\n");
    printf("\n");
    printf("[mandatory]\n");
    printf("\n");
    printf(" -i <filename> input filename\n");
    printf("                  More than one file can be converted.\n");
    printf("                  Supported formats: \n");
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
    printf(" -B <color>    select this color as background (color index 0)\n");
    printf("                valid values for <color>:\n");
    for (i = 0; i < sizeof(COLORS) / sizeof(NamedRGB); ++i) {
        printf("                %12.12s (hex: 0x%2.2x%2.2x%2.2x - R: %d, G: %d, B: %d)\n",
            COLORS[i].name,
            COLORS[i].color.red, COLORS[i].color.green, COLORS[i].color.blue,
            COLORS[i].color.red, COLORS[i].color.green, COLORS[i].color.blue
            );
    }
    printf(" -b <number>   set the bank number (used only with '-g')\n");
    printf(" -d            enable debugging (used only with '-v')\n");
    printf(" -g <filename> generate C headers of tile offsets \n");
    printf(" -l <lum>      threshold luminance\n");
    printf(" -m            enable multicolor support\n");
    printf(" -R            reverse luminance threshold\n");
    printf(" ");

    exit(_level);

}

// This function allows to parse the options entered on the command line. 
// Options must start with a minus character ('-') and only the first letter 
// is considered.

void parse_options(int _argc, char* _argv[]) {

    // Used as index.
    int i, j, c;

    // We check for each option...
    for (i = 1; i < _argc; ++i) {

        // Parse it only if begins with '-'
        if (_argv[i][0] == '-') {

            switch (_argv[i][1]) {
                case 'i': // "-i <filename>"
                    filename_in[filename_in_count++] = _argv[i + 1];
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
                case 'b': // "-b <number>"
                    configuration.bank = atoi(_argv[i + 1]);
                    ++i;
                    break;
                case 'B': // "-B <color>"
                    c = sizeof(COLORS) / sizeof(NamedRGB);
                    for (j = 0; j < c; ++j) {
                        if (stricmp(_argv[i + 1], COLORS[j].name) == 0) {
                            configuration.background = j;
                            break;
                        }
                    }
                    if (j == c) {
                        printf("Unknown color: %s", _argv[i+1]);
                        usage_and_exit(ERL_WRONG_OPTIONS, _argc, _argv);
                    }
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
                case 'd': // "-d"
                    debug = 1;
                    break;
                case 'm': // "-m"
                    configuration.multicolor = 1;
                    break;
                case 'g': // "-g"
                    filename_header = _argv[i + 1];
                    ++i;
                    break;
                default:
                    fprintf(stderr, "ERROR:: unknown option '%s'.\n", _argv[i]);
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

// This function calculates the color distance between two colors(_a and _b).
// By "distance" we mean the geometric distance between two points in a 
// three-dimensional space, where each dimension corresponds to one of the 
// components (red, green and blue). The returned value is normalized to 
// the nearest 8-bit value.

int calculate_distance(RGB _a, RGB _b) {

    // Extract the vector's components.
    double red = (double)_a.red - (double)_b.red;
    double green = (double)_a.green - (double)_b.green;
    double blue = (double)_a.blue - (double)_b.blue;

    // Calculate distance using Pitagora's Theorem
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

    int previous_tiles_count = _output->tiles_count;
    int actual_tiles_count = _configuration->width_tiles * _configuration->height_tiles;

    if (_output->tiles_count == 0) {

        // Calculate the surface area, in terms of tiles
        _output->tiles_count = actual_tiles_count;

        // Allocate enough memory
        _output->tiles = malloc(_output->tiles_count * 8);

        // Clear the tiles.
        memset(_output->tiles, 0, _output->tiles_count * 8);

    } else {

        // Update the surface area, in terms of tiles
        _output->tiles_count += actual_tiles_count;

        // Reallocate memory
        _output->tiles = realloc(_output->tiles, _output->tiles_count * 8);

        // Clear the tiles.
        memset(_output->tiles + previous_tiles_count*8, 0, actual_tiles_count * 8);

    }

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
                    *(_output->tiles + ( previous_tiles_count * 8 ) + offset) &= ~bitmask;
                    if (verbose) {
                        printf(" ");
                    }
                }
                else {
                    *(_output->tiles + ( previous_tiles_count * 8 ) + offset) |= bitmask;
                    if (verbose) {
                        printf("*");
                    }
                }
            }
            else {

                // Inversion of "on"-"off" meaning.
                if (!_configuration->reverse) {
                    *(_output->tiles + ( previous_tiles_count * 8 ) + offset) &= ~bitmask;
                    if (verbose) {
                        printf(" ");
                    }
                }
                else {
                    *(_output->tiles + ( previous_tiles_count * 8 ) + offset) |= bitmask;
                    if (verbose) {
                        printf("*");
                    }
                }
            }

            _source += _configuration->depth;

        }
        if (verbose) {
            printf("\n");
        }

    }

    if (verbose) {
        printf("\n");
        printf("\n");
    }
}

// This function extract the "palette" of colors of the given image.
int extract_color_palette(unsigned char* _source, Configuration* _configuration, RGB _palette[], int _palette_size) {

    RGB rgb;

    int image_x, image_y;

    int usedPalette = 0;
    int i = 0;
    unsigned char* source = _source;

    if (verbose&&debug) {
        printf("\nExtracting color palette from source image.\n\n");
    }

    for (image_y = 0; image_y < _configuration->height; ++image_y) {
        for (image_x = 0; image_x < _configuration->width; ++image_x) {
            rgb.red = *source;
            rgb.green = *(source + 1);
            rgb.blue = *(source + 2);

            for (i = 0; i < usedPalette; ++i) {
                if (_palette[i].red == rgb.red && _palette[i].green == rgb.green && _palette[i].blue == rgb.blue) {
                    break;
                }
            }

            if (i >= usedPalette) {
                if (verbose&&debug) {
                    printf(" ");
                }
                _palette[usedPalette].red = rgb.red;
                _palette[usedPalette].green = rgb.green;
                _palette[usedPalette].blue = rgb.blue;
                ++usedPalette;
                if (usedPalette > _palette_size) {
                    break;
                }
            } else {
                if (verbose && debug) {
                    printf("*");
                }
            }
            source += 3;
        }
        if (verbose) {
            printf("\n");
        }
        if (usedPalette > _palette_size) {
            break;
        }
    }

    if (verbose && debug) {
        printf("\n\nDetected %d different colors.\n", usedPalette);
        for (i = 0; i < usedPalette; ++i) {
            printf("%d) 0x%02.2x%02.2x%02.2x\n", i, _palette[i].red, _palette[i].green, _palette[i].blue );
        }
    }

    return usedPalette;

}

// This function convert an image of (W,H) pixels in a set of (WT,HT) multicolor
// tiles. Each tile will have the half of horizontal resolution but four colors
// for each pixel. Tiles will be drawn in a "contiguous" way, i.e. each row of 
// multicolor tiles will be drawn sequentially, and each column for each row 
// the same. 
void convert_image_into_multicolor_tiles(unsigned char* _source, Configuration* _configuration, Output* _output) {

    // Position of the pixel in the original image
    int image_x, image_y;

    // Position of the pixel, in terms of tiles
    int tile_x, tile_y;

    // Position of the pixel, in terms of offset and bitmask
    int offset, bitmask;

    // Color of the pixel to convert
    RGB rgb;

    int previous_tiles_count = _output->tiles_count;
    int actual_tiles_count = _configuration->width_tiles * _configuration->height_tiles;

    int i = 0;

    // Normalize the input image based on colors.
    RGB palette[256];
    int usedPalette = 0;
    int minDistance, colorIndex;

    usedPalette = extract_color_palette(_source, _configuration, palette, 256);

    if (_output->tiles_count == 0) {

        // Calculate the surface area, in terms of tiles
        _output->tiles_count = actual_tiles_count;

        // Allocate enough memory
        _output->tiles = malloc(_output->tiles_count * 8);

        // Clear the tiles.
        memset(_output->tiles, 0, _output->tiles_count * 8);

    }
    else {

        // Update the surface area, in terms of tiles
        _output->tiles_count += actual_tiles_count;

        // Reallocate memory
        _output->tiles = realloc(_output->tiles, _output->tiles_count * 8);

        // Clear the tiles.
        memset(_output->tiles + previous_tiles_count * 8, 0, actual_tiles_count * 8);

    }

    // Loop for all the source surface.
    for (image_y = 0; image_y < _configuration->height; ++image_y) {
        for (image_x = 0; image_x < _configuration->width; ++image_x) {

            // Take the color of the pixel
            rgb.red = *_source;
            rgb.green = *(_source + 1);
            rgb.blue = *(_source + 2);

            // Calculate the relative tile
            tile_y = (image_y >> 3);
            tile_x = (image_x >> 2);

            // Calculate the offset starting from the tile surface area
            // and the bit to set.
            offset = (tile_y * 8 * _configuration->width_tiles) + (tile_x * 8) + (image_y & 0x07);

            minDistance = 0xffff;
            colorIndex = 0;

            for (i = 0; i < 4; ++i) {
                if (calculate_distance(rgb, palette[i]) < minDistance) {
                    minDistance = calculate_distance(rgb, palette[i]);
                    colorIndex = i;
                };
            }

            bitmask = colorIndex << (6 - ((image_x & 0x3) * 2));

            *(_output->tiles + (previous_tiles_count * 8) + offset) |= bitmask;

            if (verbose) {
                printf("%1.1d", colorIndex, bitmask);
            }

            _source += _configuration->depth;

        }
        if (verbose) {
            printf("\n");
        }

    }

    if (verbose) {
        printf("\n");
        printf("\n");
    }
}

// Main function
int main(int _argc, char *_argv[]) {

    int i = 0, j = 0, k = 0, m = 0;

    parse_options(_argc, _argv);

    if (filename_in_count == 0 ) {
        fprintf(stderr, "ERROR:: missing input filename.\n");
        usage_and_exit(ERL_MISSING_INPUT_FILENAME, _argc, _argv);
    }

    if (filename_out == NULL) {
        fprintf(stderr, "ERROR:: missing output filename for luminance.\n");
        usage_and_exit(ERL_MISSING_OUTPUT_FILENAME, _argc, _argv);
    }

    if (verbose) {
        for (i = 0; i < filename_in_count; ++i) {
            printf("Input image ................. %s\n", filename_in[i]);
        }
        printf("Output tile(s) .............. %s\n", filename_out);
    }

    RGB palette[256];
    int nearestColorIndex[4];

    Output result;
    result.tiles_count = 0;

    for (i = 0; i < filename_in_count; ++i) {

        configuration.width = 0;
        configuration.height = 0;
        configuration.depth = 3;

        unsigned char* source = stbi_load(filename_in[i], &configuration.width, &configuration.height, &configuration.depth, 0);

        if (source == NULL) {
            fprintf(stderr, "ERROR:%s: unable to open file\n", filename_in[i]);
            usage_and_exit(ERL_CANNOT_OPEN_INPUT, _argc, _argv);
        }

        if (configuration.multicolor) {
            if ((configuration.width & 0x03) != 0) {
                fprintf(stderr, "ERROR:%s: cannot convert images with width (%d) not multiple of 4 pixels.\n", filename_in[i], configuration.width);
                usage_and_exit(ERL_CANNOT_CONVERT_WIDTH, _argc, _argv);
            }
            configuration.width_tiles = configuration.width >> 2;
        } else {
            if ((configuration.width & 0x07) != 0) {
                fprintf(stderr, "ERROR:%s: cannot convert images with width (%d) not multiple of 8 pixels.\n", filename_in[i], configuration.width);
                usage_and_exit(ERL_CANNOT_CONVERT_WIDTH, _argc, _argv);
            }
            configuration.width_tiles = configuration.width >> 3;
        }

        if (configuration.multicolor) {
            if ((configuration.height & 0x03) != 0) {
                fprintf(stderr, "ERROR:%s: cannot convert images with height (%d) not multiple of 8 pixels.\n", filename_in[i], configuration.height);
                usage_and_exit(ERL_CANNOT_CONVERT_HEIGHT, _argc, _argv);
            }
            configuration.height_tiles = configuration.height >> 3;
        } else {
            if ((configuration.height & 0x07) != 0) {
                fprintf(stderr, "ERROR:%s: cannot convert images with height (%d) not multiple of 8 pixels.\n", filename_in[i], configuration.height);
                usage_and_exit(ERL_CANNOT_CONVERT_HEIGHT, _argc, _argv);
            }
            configuration.height_tiles = configuration.height >> 3;
        }

        if (configuration.multicolor) {
            if (extract_color_palette(source, &configuration, palette, 256) > 4) {
                fprintf(stderr, "ERROR:%s: cannot convert images with more than 4 colors.\n", filename_in[i]);
                usage_and_exit(ERL_CANNOT_CONVERT_COLORS, _argc, _argv);
            }
            if (verbose && debug) {
                printf("\n\nCalculating nearest colors.\n");
            }
            if (configuration.background != -1) {
                if (verbose && debug) {
                    printf("\n\nStarting from background color.\n");
                }
                int minDistance = 0xffff;
                int minColorIndex = 0, distance = 0;
                for (k = 0; k < 4; ++k) {
                    distance = calculate_distance(palette[k], COLORS[configuration.background].color);
                    if (verbose && debug) {
                        printf("%d) 0x%02.2x%02.2x%02.2x => (%d) => %20.20s] 0x%02.2x%02.2x%02.2x\n", configuration.background, 
                            palette[k].red, palette[k].green, palette[k].blue,
                            distance,
                            COLORS[configuration.background].name, COLORS[configuration.background].color.red, COLORS[configuration.background].color.green, COLORS[configuration.background].color.blue);
                    }

                    if (distance < minDistance) {
                        minColorIndex = k;
                        minDistance = distance;
                    }
                }
                RGB temp = palette[minColorIndex];
                palette[minColorIndex] = palette[0];
                palette[0] = temp;
            }
            for (j = 0; j < 4; ++j) {
                int minDistance = 0xffff;
                int minColorIndex = 0, distance = 0;
                for (k = 0; k < sizeof(COLORS) / sizeof(NamedRGB); ++k) {
                    distance = calculate_distance(palette[j], COLORS[k].color);
                    if (verbose && debug) {
                        printf("%d) %20.20s] 0x%02.2x%02.2x%02.2x => (%d) => 0x%02.2x%02.2x%02.2x\n", j, COLORS[k].name,
                            palette[j].red, palette[j].green, palette[j].blue,
                            distance,
                            COLORS[k].color.red, COLORS[k].color.green, COLORS[k].color.blue);
                    }

                    if (distance < minDistance) {
                        for (m = 0; m < j; ++m) {
                            if (nearestColorIndex[m] == k) {
                                break;
                            }
                        }
                        if (m >= j) {
                            minColorIndex = k;
                            minDistance = distance;
                        }
                    }
                }
                if (verbose && debug) {
                    printf("\n");
                    printf("%d) 0x%02.2x%02.2x%02.2x => (%d) => 0x%02.2x%02.2x%02.2x\n", j,
                            palette[j].red, palette[j].green, palette[j].blue, 
                            minDistance,
                            COLORS[minColorIndex].color.red, COLORS[minColorIndex].color.green, COLORS[minColorIndex].color.blue);
                }
                nearestColorIndex[j] = minColorIndex;
            }
            if (verbose && debug) {
                printf("\n");
            }
        }

        if (verbose) {
            printf(" %s: (%dx%d, %d bpp) -> (%dx%d, %d bpp)\n", filename_in[i], configuration.width, configuration.height, configuration.depth, configuration.width_tiles, configuration.height_tiles, 1+configuration.multicolor );
        }

        width_in_tiles[i] = configuration.width_tiles;
        height_in_tiles[i] = configuration.height_tiles;
        starting_tile[i] = result.tiles_count;

        if (configuration.multicolor) {
            convert_image_into_multicolor_tiles(source, &configuration, &result);
        } else {
            convert_image_into_tiles(source, &configuration, &result);
        }

        stbi_image_free(source);

    }

    FILE *handle = fopen(filename_out, "w+b");
    if (handle == NULL) {
        fprintf(stderr, "ERROR:: unable to open output file '%s'.\n", filename_out);
        usage_and_exit(ERL_CANNOT_OPEN_OUTPUT, _argc, _argv);
    }

    fwrite(result.tiles, 8, result.tiles_count, handle);
    fclose(handle);

    if (filename_header != NULL) {
        unsigned char buffer[80];
        sprintf(buffer, "%d", 0);
        FILE* handle = fopen(filename_header, "w+t");
        if (handle == NULL) {
            fprintf(stderr, "ERROR:: unable to open header file %s\n", filename_header);
            usage_and_exit(ERL_CANNOT_OPEN_HEADER, _argc, _argv);
        }
        if (configuration.bank > 0) {
            fprintf(handle, "#ifndef _TILES%d_\n", configuration.bank);
            fprintf(handle, "\n\t#define TILE%d_START%*s\n", configuration.bank, 35, buffer);
        }
        else {
            fprintf(handle, "#ifndef _TILES_\n");
            fprintf(handle, "\n\t#define TILE_START%*s\n", 35, buffer);
        }
        if (configuration.multicolor) {
            for (i = 0; i < 4; ++i) {
                if (configuration.bank > 0) {
                    fprintf(handle, "\n\t#define TILE%d_COLOR%d%*sMR_COLOR_%s", configuration.bank, i, 33, " ", COLORS[nearestColorIndex[i]].name);
                } else {
                    fprintf(handle, "\n\t#define TILE_COLOR%d%*sMR_COLOR_%s", i, 33, " ", COLORS[nearestColorIndex[i]].name);
                }
            }
        }
        fprintf(handle, "\n");
        for (i = 0; i < filename_in_count; ++i) {
            unsigned char* tilename = basename(filename_in[i]);
            unsigned char* sep = strrchr(tilename, '_');
            unsigned char* dot = strchr(tilename, '.');
            *dot = 0;
            if (sep == NULL) sep = tilename;
            ++sep;
            sep = strupr(sep);
            sprintf(buffer, "%d", starting_tile[i]);
            if (configuration.bank > 0) {
                fprintf(handle, "\n\t#define TILE%d_%s%*s\n", configuration.bank, sep, (40 - strlen(sep)), buffer);
            } else {
                fprintf(handle, "\n\t#define TILE_%s%*s\n", sep, (40 - strlen(sep)), buffer);
            }
            sprintf(buffer, "%d", width_in_tiles[i]);
            if (configuration.bank > 0) {
                fprintf(handle, "\t#define TILE%d_%s_WIDTH%*s\n", configuration.bank, sep, (34 - strlen(sep)), buffer);
            } else {
                fprintf(handle, "\t#define TILE_%s_WIDTH%*s\n", sep, (34 - strlen(sep)), buffer);
            }
            sprintf(buffer, "%d", height_in_tiles[i]);
            if (configuration.bank > 0) {
                fprintf(handle, "\t#define TILE%d_%s_HEIGHT%*s\n", configuration.bank, sep, (33 - strlen(sep)), buffer);
            } else {
                fprintf(handle, "\t#define TILE_%s_HEIGHT%*s\n", sep, (33 - strlen(sep)), buffer);
            }
        }
        sprintf(buffer, "%d", result.tiles_count);
        if (configuration.bank > 0) {
            fprintf(handle, "\n\t#define TILE%d_COUNT%*s\n", configuration.bank, 36, buffer);
        } else {
            fprintf(handle, "\n\t#define TILE_COUNT%*s\n", 36, buffer);
        }
        fprintf(handle, "#endif\n");
        fclose(handle);
    }

    if (verbose) {
        printf("Wrote a total of %d tiles.\n\n", result.tiles_count);
    }

}