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

#include <stdarg.h>
#include "bitmap.h"

int
init_24bit_header(
    bitmap_file_header  *file_header,
    bitmap_info_header  *info_header,
    int                 width,
    int                 height
) {
    file_header->bf_size = sizeof(bitmap_file_header) + sizeof(bitmap_info_header) + sizeof(bitmap_24bit_pixel) * width * height;
    file_header->bf_offset = sizeof(bitmap_file_header) + sizeof(bitmap_info_header);

    info_header->bi_header_size = sizeof(bitmap_info_header);
    info_header->bi_width = width;
    info_header->bi_height = height;
    info_header->bi_bit_size = 24;
    info_header->bi_data_size = sizeof(bitmap_24bit_pixel) * width * height;

    return FUNCTION_SUCCESS;
}

int set_24bit_pixel_color(bitmap_24bit_pixel *pixel, uint8_t red, uint8_t green, uint8_t blue) {
    pixel->b24p_red = red;
    pixel->b24p_green = green;
    pixel->b24p_blue = blue;

    return FUNCTION_SUCCESS;
}
