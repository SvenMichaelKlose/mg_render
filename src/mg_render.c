
/*
 *  mg_render.c - written 2020-09-17 by Michael Kircher
 *
 *  - validate and render a MG picture file to *.ppm with predefined palette.
 */

#include <stdio.h>
#include <stdlib.h>
#include "image.h"

#define X_SIZE 160
#define Y_SIZE 192

#define FILE_SIZE 4097

typedef enum { false, true } boolean;

RGB vic_palette[16] = {
    {0, 0, 0},                  /* Black        */
    {255, 255, 255},            /* White        */
    {148, 23, 1},               /* Red          */
    {82, 198, 212},             /* Cyan         */
    {144, 28, 204},             /* Purple       */
    {81, 190, 8},               /* Green        */
    {49, 26, 213},              /* Blue         */
    {196, 215, 28},             /* Yellow       */
    {163, 81, 0},               /* Orange       */
    {208, 176, 22},             /* Light Orange */
    {233, 121, 105},            /* Light Red    */
    {132, 243, 255},            /* Light Cyan   */
    {221, 115, 255},            /* Light Purple */
    {142, 246, 79},             /* Light Green  */
    {133, 112, 255},            /* Light Blue   */
    {242, 255, 84}              /* Light Yellow */
};

unsigned char MG_FILE_HEADER[] = {
    0xF1, 0x10, 0x0C, 0x12, 0xD8, 0x07, 0x9E, 0x20, 0x38, 0x35,
    0x38, 0x34, 0x00, 0x00, 0x00
};

unsigned char MG_FILE_FOOTER[] = {
    0x18, 0xA9, 0x10, 0xA8, 0x99, 0xF0, 0x0F, 0x69, 0x0C, 0x90,
    0x02, 0xE9, 0xEF, 0xC8, 0xD0, 0xF4, 0xA0, 0x05, 0x18, 0xB9,
    0xE4, 0xED, 0x79, 0xFA, 0x21, 0x99, 0x00, 0x90, 0x88, 0x10,
    0xF3, 0xAD, 0x0E, 0x90, 0x29, 0x0F, 0x0D, 0x0E, 0x12, 0x8D,
    0x0E, 0x90, 0xAD, 0x0F, 0x12, 0x8D, 0x0F, 0x90, 0xA9, 0x10,
    0x85, 0xFB, 0xA9, 0x12, 0x85, 0xFC, 0xA9, 0x00, 0x85, 0xFD,
    0xA9, 0x11, 0x85, 0xFE, 0xA2, 0x0F, 0xA0, 0x00, 0xB1, 0xFB,
    0x91, 0xFD, 0xC8, 0xD0, 0xF9, 0xE6, 0xFC, 0xE6, 0xFE, 0xCA,
    0xD0, 0xF2, 0xA2, 0x00, 0xA0, 0x00, 0xBD, 0x10, 0x21, 0xE8,
    0x99, 0x00, 0x94, 0xC8, 0x4A, 0x4A, 0x4A, 0x4A, 0x99, 0x00,
    0x94, 0xC8, 0xC0, 0xF0, 0xD0, 0xEC, 0x20, 0xE4, 0xFF, 0xF0,
    0xFB, 0x4C, 0x32, 0xFD, 0x02, 0xFE, 0xFE, 0xEB, 0x00, 0x0C
};

IMAGE *
mg_render (unsigned char *raw)
{
    IMAGE *dst;

    if (!(dst = image_alloc (X_SIZE, Y_SIZE, RGB_IMG)))
        return NULL;

    RGB temp, palette[4];
    int x1, x2, y1, y2;
    unsigned char bitmap, colour;
    boolean is_multi, is_inverse = (0 == (raw[-1] & 0x08));

    palette[3] = vic_palette[(raw[-2] >> 4) & 0x0F];
    for (y1 = 0; y1 < Y_SIZE / 16; y1++) {
        for (x1 = 0; x1 < X_SIZE / 8; x1++) {
            colour =
                (raw + X_SIZE * Y_SIZE / 8)[((X_SIZE / 8) * y1 + x1) / 2];
            palette[0] = vic_palette[(raw[-1] >> 4) & 0x0F];
            if (0 == (x1 & 1)) {
                palette[1] = vic_palette[colour & 0x07];
                is_multi = (0 != (colour & 0x08));
            } else {
                palette[1] = vic_palette[(colour >> 4) & 0x07];
                is_multi = (0 != ((colour >> 4) & 0x08));
            }
            palette[2] = vic_palette[raw[-1] & 0x07];
            if (is_multi) {
                temp = palette[1];
                palette[1] = palette[2];
                palette[2] = temp;
            } else if (is_inverse) {
                temp = palette[0];
                palette[0] = palette[1];
                palette[1] = temp;
            }
            for (y2 = 0; y2 < 16; y2++) {
                bitmap = raw[Y_SIZE * x1 + 16 * y1 + y2];
                if (!is_multi)
                    for (x2 = 0; x2 < 8; x2 += 1)
                        rgb_pixel (dst, 8 * x1 + x2, 16 * y1 + y2) =
                            palette[(bitmap >> (7 - x2)) & 0x01];
                else
                    for (x2 = 0; x2 < 8; x2 += 2) {
                        int x = 8 * x1 + x2, y = 16 * y1 + y2;

                        rgb_pixel (dst, x + 1, y) =
                            rgb_pixel (dst, x, y) =
                            palette[(bitmap >> (6 - x2)) & 0x03];
                    }
            }
        }
    }

    return dst;
}

int
main (int argc, char *argv[])
{
    FILE *src;
    IMAGE *dst;
    unsigned char file[FILE_SIZE];
    int i, data;

    if (3 != argc) {
        fprintf (stderr, "%s: Syntax: %s <in_file> <out_file>\n", argv[0],
                 argv[0]);
        exit (EXIT_FAILURE);
    }

    if (!(src = fopen (argv[1], "rb"))) {
        fprintf (stderr,
                 "%s: Error: could not open file \'%s\' for read access.\n",
                 argv[0], argv[1]);
        exit (EXIT_FAILURE);
    }

    for (i = 0; i < FILE_SIZE; i++) {
        if (EOF == (data = getc (src))) {
            fprintf (stderr, "%s: Error: file \'%s\' truncated.\n", argv[0],
                     argv[1]);
            exit (EXIT_FAILURE);
        }
        file[i] = data;
    }
    if (EOF != (data = getc (src))) {
        fprintf (stderr, "%s: Error: file \'%s\' over-length.\n", argv[0],
                 argv[1]);
        exit (EXIT_FAILURE);
    }

    fclose (src);

    for (i = 0; i < sizeof (MG_FILE_HEADER); i++) {
        if (file[i] != MG_FILE_HEADER[i]) {
            fprintf (stderr, "%s: Error: file \'%s\', header mismatch.\n",
                     argv[0], argv[1]);
            exit (EXIT_FAILURE);
        }
    }
    for (i = 0; i < sizeof (MG_FILE_FOOTER); i++) {
        if (file[i + FILE_SIZE - sizeof (MG_FILE_FOOTER)] !=
            MG_FILE_FOOTER[i]) {
            fprintf (stderr, "%s: Error: file \'%s\', footer mismatch.\n",
                     argv[0], argv[1]);
            exit (EXIT_FAILURE);
        }
    }
    if (0 != (file[15] & 0x0F)) {
        fprintf (stderr,
                 "%s: Error: file \'%s\' saved with non-0 audio volume.\n",
                 argv[0], argv[1]);
        exit (EXIT_FAILURE);
    }

    dst = mg_render (file + 17);
    image_save (dst, argv[2]);
    image_free (dst);

    exit (EXIT_SUCCESS);
}
