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
 *
 * 此文件是 Leisrasterfilter 的一部分。
 * Leisrasterfilter 是自由软件：您可以在遵照自由软件基金会发布的「GNU Affero 通用
 * 公共许可证」（第 3 版或者更新版本皆可）的前提下对其进行转载或者修改。
 * 发布 Leisrasterfilter 的初衷是希望它能有一定的用处，但是并不为销售或特定用途等
 * 情况做出任何担保。参见「GNU Affero 通用公共许可证」。
 * 您应该随 Leisrasterfilter 获得了一份「GNU Affero 通用公共许可证」的副本。如果
 * 没有，请看 <https://www.gnu.org/licenses/agpl-3.0.txt>。
 */

// #include <stdarg.h>
#include "bitmap.h"

/*
 * 打算把所有的 log 信息都输出在 stderr，以免标准输出流被重定向到文件或其他位置时
 * 输出无关信息。
 */

/*
 * log_error() - 将错误信息输出到 stderr。
 */
void
log_error(              /* 输出 - void */
    char    *type,      /* 输入 - 信息类型 */
    char    *content    /* 输入 - 信息内容 */
) {
    fprintf(stderr, "[!!] %s: %s\n", type, content);
}

/*
 * log_error() - 将调试信息输出到 stderr。
 */
void
log_debug(              /* 输出 - void */
    char    *type,      /* 输入 - 信息类型 */
    char    *content    /* 输入 - 信息内容 */
) {
    fprintf(stderr, "[++] %s: %s\n", type, content);
}

/*
 * init_24bit_header() - 初始化 24 位深 bitmap 文件的完整头部信息。
 */
int
init_24bit_header(      /* 输出 - 1 成功, 0 失败 */
    bitmap_file_header  *file_header,   /* 输入 - 文件头部信息 */
    bitmap_info_header  *info_header,   /* 输入 - 位图头部信息 */
    unsigned int        width,          /* 输入 - 图像宽度 */
    unsigned int        height          /* 输入 - 图像高度 */
) {
    /* bitmap 内容数据中，要求每行的字节数是 4 的倍数，计算出每行缺少的字符数。 */
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
    info_header->bi_x_res = BITMAP_INFO_DEFAULT_X_RES;
    info_header->bi_y_res = BITMAP_INFO_DEFAULT_Y_RES;
    info_header->bi_color_index = BITMAP_INFO_DEFAULT_COLOR_INDEX;
    info_header->bi_color_important = BITMAP_INFO_DEFAULT_COLOR_IMPORTANT;

    return FUNCTION_SUCCESS;
}

/*
 * set_24bit_pixel_color() - 将一个像素的 R, G, B 数值转换为像素点的属性值。
 */
int                             /* 输出 - 1 成功, 0 失败 */
set_24bit_pixel_color(
    bitmap_24bit_pixel  *pixel, /* 输入 - 一个 24 位深像素点 */
    uint8_t             red,    /* 输入 - 8 位红色值 */
    uint8_t             green,  /* 输入 - 8 位绿色值 */
    uint8_t             blue    /* 输入 - 8 位蓝色值 */
) {
    pixel->b24p_red = red;
    pixel->b24p_green = green;
    pixel->b24p_blue = blue;

    return FUNCTION_SUCCESS;
}

/*
 * bitmap_24bit_write() - 向流对象写入一个完整的 bitmap 文件。
 */
int                                     /* 输出 - 1 成功, 0 失败 */
bitmap_24bit_write(
    bitmap_file_header  file_header,    /* 输入 - 文件头部信息 */
    bitmap_info_header  info_header,    /* 输入 - 位图头部信息 */
    bitmap_24bit_pixel  *pixels,        /* 输入 - 像素点阵 */
    void                *fp             /* 输入 - 流对象 */
) {
    int                 failure = FUNCTION_SUCCESS;

    /* bitmap 内容数据中，要求每行的字节数是 4 的倍数，这是用于填充空白部分的随机信息。 */
    char                str_to_fill[3] = {70, 82, 76};

    unsigned int        width = info_header.bi_width;
    unsigned int        height = info_header.bi_height;
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

/*
 * init_job() - 模拟一个打印服务器，初始化一个打印任务。
 */
int                                 /* 输出 - 1 成功，0 失败 */
init_job(
    int                 argc,       /* 输入 - 从 main() 传过来的参数个数 */
    char                *argv[],    /* 输入 - 从 main() 传过来的参数内容 */
    bitmap_job_data_t   *job        /* 输入 - 一个任务对象 */
) {
    int i;

    /* 检查命令行参数个数。 */
    if ( argc != 7 ) {
        log_error("Error", "Arguments wrong!");
        fprintf(stderr, "argc = %d\n", argc);
        for ( i = 0; i < argc; i ++ ) {
            fprintf(stderr, "argv[%d] = %s\n", i, argv[i]);
        }
        /*                    0  1    2     3   4 5  6                  */
        fprintf(stderr, "用法：%s 任务 用户名 标题 选项 [文件名]\n", argv[0]);
        return FUNCTION_FAILURE;
    }

    /* 解析选项。 */
    job->job_id = atoi(argv[1]);
    job->user = argv[2];
    job->title = argv[3];
    job->num_options = cupsParseOptions(argv[5], 0, &( job->options ));

    return FUNCTION_SUCCESS;
}
