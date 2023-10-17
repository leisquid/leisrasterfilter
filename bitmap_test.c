/*
 * bitmap_test.c - a source code file of Leisrasterfilter
 * Copyright (c) 2023 Leisquid Li.
 *
 * This file is part of Leisrasterfilter.
 * Leisrasterfilter is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 * Leisrasterfilter is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public
 * License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with Leisrasterfilter. If not, see
 * <https://www.gnu.org/licenses/agpl-3.0.txt>.
 */

#include <stdio.h>
#include "bitmap.h"

int main(int argc, char *argv[]) {
    bitmap_file_header file_header;
    bitmap_info_header info_header;
    FILE *fp = NULL;
    bitmap_24bit_pixel pixel;

    const unsigned int width = 81;
    const unsigned int height = 81;
    bitmap_24bit_pixel pixels[width * height];

    int index;

    // printf("%ld\n", sizeof(bitmap_file_header));

    init_24bit_header(&file_header, &info_header, width, height);

    set_24bit_pixel_color(&pixel, 0x00, 0x80, 0xff);

    for (index = 0; index < width * height; index ++) {
        pixels[index] = pixel;
    }

    fp = fopen("test.bmp", "wb");

    if ( bitmap_24bit_write(file_header, info_header, pixels, fp) != FUNCTION_SUCCESS ) {
        log_error("ERROR", "File writing failure!");
    }

    fclose(fp);

    return EXIT_SUCCESS;
}
