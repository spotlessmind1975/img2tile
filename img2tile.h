/*****************************************************************************
 * IMG2TILE - Utility to convert images into (a set of) tile(s)              *
 *****************************************************************************
 * Copyright 2020 Marco Spedaletti (asimov@mclink.it)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *----------------------------------------------------------------------------
 * Concesso in licenza secondo i termini della Licenza Apache, versione 2.0
 * (la "Licenza"); è proibito usare questo file se non in conformità alla
 * Licenza. Una copia della Licenza è disponibile all'indirizzo:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Se non richiesto dalla legislazione vigente o concordato per iscritto,
 * il software distribuito nei termini della Licenza è distribuito
 * "COSÌ COM'È", SENZA GARANZIE O CONDIZIONI DI ALCUN TIPO, esplicite o
 * implicite. Consultare la Licenza per il testo specifico che regola le
 * autorizzazioni e le limitazioni previste dalla medesima.
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

    // This structure stores the name and the color components (red, blue 
    // and green) of a pixel, 8 bits wide. This structure is used both to 
    // represent the retrocomputer palette and to process input data from 
    // image files.
    typedef struct {
        char name[32];
        RGB color;
    } NamedRGB;

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

        int multicolor;

        int background;

    } Configuration;

    // This structure maintain the result of conversion operation.

    typedef struct {

        mr_tile     tiles_count;
        mr_mixel*   tiles;

    } Output;

#endif
