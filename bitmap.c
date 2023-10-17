/*
 * bitmap.c - a source code file of Leisrasterfilter
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

// #include <stdarg.h>
#include "bitmap.h"

void
log_error(
    char    *type,
    char    *content
) {
    fprintf(stderr, "[!!] %s: %s\n", type, content);
}

void
log_debug(
    char    *type,
    char    *content
) {
    fprintf(stdout, "[++] %s: %s\n", type, content);
}

int
init_24bit_header(
    bitmap_file_header  *file_header,
    bitmap_info_header  *info_header,
    unsigned int        width,
    unsigned int        height
) {
    int width_to_fill = ( (width * 3 % 4)? (4 - (width * 3 % 4)): 0 );

    file_header->bf_type = BITMAP_FILE_TYPE_LE;
    file_header->bf_size = sizeof(bitmap_file_header)
                         + sizeof(bitmap_info_header)
                         + sizeof(bitmap_24bit_pixel) * width * height
                         + width_to_fill * height;
    file_header->bf_reserved1 = BITMAP_FILE_RESERVED1;
    file_header->bf_reserved2 = BITMAP_FILE_RESERVED2;
    file_header->bf_offset = sizeof(bitmap_file_header) + sizeof(bitmap_info_header);

    info_header->bi_header_size = sizeof(bitmap_info_header);
    info_header->bi_width = width;
    info_header->bi_height = height;
    info_header->bi_color_plane = BITMAP_INFO_DEFAULT_COLOR_PLANE;
    info_header->bi_bit_size = 24;
    info_header->bi_compression = BITMAP_INFO_NON_COMPRESSION;
    info_header->bi_data_size = sizeof(bitmap_24bit_pixel) * width * height + width_to_fill * height;
    info_header->bi_x_res = 0;
    info_header->bi_y_res = 0;
    info_header->bi_color_index = BITMAP_INFO_DEFAULT_COLOR_INDEX;
    info_header->bi_color_important = BITMAP_INFO_DEFAULT_COLOR_IMPORTANT;

    return FUNCTION_SUCCESS;
}

int set_24bit_pixel_color(bitmap_24bit_pixel *pixel, uint8_t red, uint8_t green, uint8_t blue) {
    pixel->b24p_red = red;
    pixel->b24p_green = green;
    pixel->b24p_blue = blue;

    return FUNCTION_SUCCESS;
}

int
bitmap_24bit_write(
    bitmap_file_header  file_header,
    bitmap_info_header  info_header,
    bitmap_24bit_pixel  *pixels,
    FILE                *fp
) {
    int                 failure = FUNCTION_SUCCESS;
    char                str_to_fill[3] = {70, 82, 76};

    unsigned int        width = info_header.bi_width;
    unsigned int        height = info_header.bi_height;
    // unsigned long long  pixels_count = width * height;
    unsigned long long  bytes_count = 0;
    unsigned int        index;
    unsigned int        jndex;  /* （笑） */
    

    if ( fwrite(&file_header, sizeof(bitmap_file_header), 1, fp) != 1 ) {
        failure = FUNCTION_FAILURE;
        return failure;
    }

    if ( fwrite(&info_header, sizeof(bitmap_info_header), 1, fp) != 1 ) {
        failure = FUNCTION_FAILURE;
        return failure;
    }

    for ( index = 0 ; index < height; index ++ ) {
        if ( failure == FUNCTION_FAILURE ) {
            break;
        }

        /* 写一行。 */
        for ( jndex = 0; jndex < width; jndex ++ ) {
            if ( failure == FUNCTION_FAILURE ) {
                break;
            }

            if ( fwrite(&(pixels[index * height + jndex]), sizeof(bitmap_24bit_pixel), 1, fp) != 1 ) {
                failure = FUNCTION_FAILURE;
            } else {
                bytes_count += sizeof(bitmap_24bit_pixel);
            }
        }

        while ( bytes_count % 4 != 0 ) {
            if ( fwrite(&(str_to_fill[bytes_count % 4 - 1]), sizeof(char), 1, fp) != 1 ) {
                failure = FUNCTION_FAILURE;
            } else {
                bytes_count += sizeof(char);
            }
        }
    }

    return failure;
}
