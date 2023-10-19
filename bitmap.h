/*
 * bitmap.h - a source code file of Leisrasterfilter
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

#ifndef __LEISRASTERFILTER_BITMAP_H
#define __LEISRASTERFILTER_BITMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cups/cups.h>
#include <string.h>
#include <fcntl.h>

#ifndef FUNCTION_SUCCESS
#define FUNCTION_SUCCESS                    1       /* 定义函数成功默认返回 1 */
#endif
#ifndef FUNCTION_FAILURE
#define FUNCTION_FAILURE                    0       /* 定义函数失败默认返回 0 */
#endif

#define BITMAP_FILE_TYPE_LE                 0x4d42  /* 位图文件标识通常为固定值，小端值为 0x4d42，即 ASCII “BM” */
#define BITMAP_FILE_TYPE_BE                 0x424d  /* 位图文件标识的大端序表示 */
#define BITMAP_FILE_RESERVED1               29516   /* 预留字段 1 的默认值 */
#define BITMAP_FILE_RESERVED2               18002   /* 预留字段 2 的默认值 */
#define BITMAP_INFO_DEFAULT_COLOR_PLANE     1       /* 色彩平面数必须为 1 */
#define BITMAP_INFO_DEFAULT_COLOR_INDEX     0       /* 颜色索引数通常为 0 */
#define BITMAP_INFO_DEFAULT_COLOR_IMPORTANT 0       /* 重要颜色数通常为 0 */
#define BITMAP_INFO_NON_COMPRESSION         0       /* 压缩方式 0 为不压缩 */
#define BITMAP_INFO_DEFAULT_X_RES           0       /* 横向分辨率的默认值 */
#define BITMAP_INFO_DEFAULT_Y_RES           0       /* 纵向分辨率的默认值 */
/*
 * 一般有的地方会说上面的这两个值可以为 0，但是在 KolourPaint 输出的文件中，这个值
 * 好像被设定为 3,780，或者叫做 0xec4。到底有什么特定含义呢？我还不太清楚。
 */

/*
 * bitmap 文件头部信息，在文件中为 14 字节。
 */
typedef struct {
    uint16_t    bf_type;        /* 位图文件标识 */
    uint32_t    bf_size;        /* 整个文件的大小 */
    uint16_t    bf_reserved1;   /* 预留字段 1 */
    uint16_t    bf_reserved2;   /* 预留字段 2 */
    uint32_t    bf_offset;      /* 内容开始位置 */
} __attribute__ ((packed)) bitmap_file_header;

/*
 * bitmap 位图头部信息，在文件中为 40 字节。
 */
typedef struct {
    uint32_t    bi_header_size;     /* 此头部信息的大小 */
    uint32_t    bi_width;           /* 图像宽度 */
    uint32_t    bi_height;          /* 图像高度 */
    uint16_t    bi_color_plane;     /* 色彩平面的数量，必须为 1 */
    uint16_t    bi_bit_size;        /* 每个像素所用位元数 */
    uint32_t    bi_compression;     /* 压缩方式 */
    uint32_t    bi_data_size;       /* 位图数据大小 */
    uint32_t    bi_x_res;           /* 横向分辨率 (px/m) */
    uint32_t    bi_y_res;           /* 纵向分辨率 (px/m) */
    uint32_t    bi_color_index;     /* 颜色索引数 */
    uint32_t    bi_color_important; /* 重要颜色数 */
} __attribute__ ((packed)) bitmap_info_header;

/*
 * bitmap 调色板信息。
 * 当位图的色深小于等于 8 时，文件头部必须带有调色板信息数据。
 */
typedef struct {
    uint8_t     bp_blue;        /* 蓝色像素值 */
    uint8_t     bp_green;       /* 绿色像素值 */
    uint8_t     bp_red;         /* 红色像素值 */
    uint8_t     bp_reserved;    /* 保留值 */
} __attribute__ ((packed)) bitmap_palette;

/*
 * bitmap 24 位像素点信息，以“蓝、绿、红”的顺序存储。
 */
typedef struct {
    uint8_t     b24p_blue;  /* 蓝色像素值 */
    uint8_t     b24p_green; /* 绿色像素值 */
    uint8_t     b24p_red;   /* 红色像素值 */
} __attribute__ ((packed)) bitmap_24bit_pixel;

/* 
 * 任务数据。
 */
typedef struct {
    int             job_id;         /* 任务 id */
    const char      *user,          /* 提交任务的用户 */
                    *title;         /* 任务标题 */
    int             num_options;    /* 命令行选项个数 */
    cups_option_t   *options;       /* 命令行选项 */
} bitmap_job_data_t;

/*
 * bitmap.h 中的函数声明。具体定义位于 ./bitmap.c 。
 */

extern int init_24bit_header(bitmap_file_header *file_header, bitmap_info_header *info_header, unsigned width, unsigned height);
extern int set_24bit_pixel_color(bitmap_24bit_pixel *pixel, uint8_t red, uint8_t green, uint8_t blue);
extern int bitmap_24bit_write(bitmap_file_header file_header, bitmap_info_header info_header, bitmap_24bit_pixel *pixels, void *fp);
extern int pixel_24bit_matrix_upsidedown(bitmap_24bit_pixel *pixels, unsigned width, unsigned height);

extern void log_error(char *type, char *content);
extern void log_debug(char *type, char *content);

extern int init_job(int argc, char *argv[], bitmap_job_data_t *job);

#endif
