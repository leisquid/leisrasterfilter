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
 *
 * 此文件是 Leisrasterfilter 的一部分。
 * Leisrasterfilter 是自由软件：您可以在遵照自由软件基金会发布的「GNU Affero 通用
 * 公共许可证」（第 3 版或者更新版本皆可）的前提下对其进行转载或者修改。
 * 发布 Leisrasterfilter 的初衷是希望它能有一定的用处，但是并不为销售或特定用途等
 * 情况做出任何担保。参见「GNU Affero 通用公共许可证」。
 * 您应该随 Leisrasterfilter 获得了一份「GNU Affero 通用公共许可证」的副本。如果
 * 没有，请看 <https://www.gnu.org/licenses/agpl-3.0.txt>。
 */

/*
 * 这是一个用于测试 bitmap 文件输出的小程序，按照设定的输出图片的宽度和高度，
 * 输出一个使用 #0080ff 颜色填充的纯色图片的 24-bit bitmap 文件。
 */

#include "bitmap.h"

const unsigned  width = 83;
const unsigned  height = 100;

/*
 * main() - 程序主入口。
 */
int                                 /* 输出 - 0 成功，1 失败 */
main(
    int argc,                       /* 输入 - 命令行参数个数。 */
    char *argv[]                    /* 输入 - 命令行参数内容。 */
) {
    bitmap_file_header file_header;
    bitmap_info_header info_header;
    FILE *fp = NULL;
    bitmap_24bit_pixel pixel;

    // bitmap_24bit_pixel pixels[(width * height)];
    bitmap_24bit_pixel *pixels
            = (bitmap_24bit_pixel *) malloc(
                sizeof(bitmap_24bit_pixel
                    [(width * (height * 2))])
                );

    int index;

    puts("A bitmap output testing tool distributed under the AGPL.");
    puts("Copyright (c) 2023 Leisquid Li.\n");

    init_24bit_header(&file_header, &info_header, width, height);

    set_24bit_pixel_color(&pixel, 0, 128, 255);

    for (index = 0; index < width * height; index ++) {
        pixels[index] = pixel;
    }

    fp = fopen("test.bmp", "wb");

    if ( bitmap_24bit_write(file_header, info_header, pixels, fp) != FUNCTION_SUCCESS ) {
        log_error("ERROR", "File writing failure!");
    }

    fclose(fp);
    free(pixels);

    puts("A bitmap file has been generated in this directory.\nBye.");

    return EXIT_SUCCESS;
}
