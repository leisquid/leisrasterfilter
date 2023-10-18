/*
 * rastertobitmap.c - a source code file of Leisrasterfilter
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

#include "bitmap.h"
#include <cups/raster.h>
#include <signal.h>

static int  CancelJob = 0;          /* 设为 1 时取消当前任务 */

int main(int argc, char *argv[]) {
    bitmap_job_data_t   job;        /* 任务数据 */
    int                 page = 0;   /* 当前页数 */
    int                 fd;         /* raster 数据的文件描述符 */
    cups_raster_t       *ras;       /* raster 流 */
    cups_page_header2_t header;     /* 当前页头数据 */
    unsigned            y;          /* 当前行 */
    unsigned char       *line;      /* 行缓冲 */

    // sleep(30);      // sleep to make it attachable by GDB

    /* 初始化操作。 */
    if ( (init_job(argc, argv, &job) ) == FUNCTION_FAILURE ) {
        log_error("Error", "Initialization failed");
        return EXIT_FAILURE;
    }

    /* 注册一个信号处理器。 */
    signal(SIGTERM, SignalHandler);

    /* 准备打印任务。 */
    if ( ! setup(&job) ) {
        return EXIT_FAILURE;
    }

    /* 打开 raster 流。 */
    if ( argc >= 6 ) {
        if ( ( fd = open(argv[6], O_RDONLY) ) == -1 ) {
            log_error("Error", "Unable to open raster file!");
        }
    } else {
        fd = 0;
    }
    ras = cupsRasterOpen(fd, CUPS_RASTER_READ);

    /* 处理页面。 */
    while ( cupsRasterReadHeader2(ras, &header) ) {
        /* 检查是否有任务取消。 */
        if ( CancelJob ) {
            break;
        }

        /* 为这一行分配内存。 */
        if ( ( line = malloc(header.cupsBytesPerLine) ) == NULL ) {
            log_error("Error", "Unable to allocate line memory!");
            break;
        }

        /* 开始打印了。 */
        page ++;
        fprintf(stderr, "PAGE: %d %d\n", page, header.NumCopies);
        log_debug("Info", "Starting page");

        if ( !start_page(&job, &header) ) {
            break;
        }

        /* 打印页面上的每一行。 */
        for (y = 0; y < header.cupsHeight; y ++) {
            /* 检查是否有任务取消。 */
            if ( CancelJob ) {
                break;
            }

            /* 显示进度。 */
            if ( ( y & 128 ) == 0 ) {
                fprintf(stderr, "++ Printing page %d, %.0f%% completed\n", page, (100.0 * y / header.cupsHeight));
                puts("LEVELS");
                fflush(stderr);
            }

            /* 读写每一行。 */
            if ( cupsRasterReadPixels(ras, line, header.cupsBytesPerLine) > 0 ) {
                if ( ! output_line(&header, line) ) {
                    break;
                }
            } else {
                break;
            }
        }

        /* 释放行内存。 */
        free(line);

        /* 显示进度并结束当前页。 */
        log_debug("Info", "Finishing page");


    }

    return EXIT_SUCCESS;
}

/*
 * setup() - 配置任务。
 */
static int                      /* 输出 - 1 成功，0 失败 */
setup(
    bitmap_job_data_t   *job    /* 输出 - 任务数据 */
) {
    fprintf(stderr, "DOCUMENT\n");
    fprintf(stderr, "AUTHOR %s\n", job->user);
    fprintf(stderr, "DOCUMENT %s\n", job->title);

    return FUNCTION_SUCCESS;
}

/*
 * StartPage() - 开始输出一页。
 */
static int
start_page(                     /* 输出 - 1 成功，0 失败 */
    bitmap_job_data_t   *job,   /* 输入 - 任务数据 */
    cups_page_header2_t *header /* 输入 - 页头 */
) {
    if (
        header->cupsBitsPerColor != 8 &&
        header->cupsBitsPerColor != 16
    ) {
        log_error("Error", "Bad cupsBitsPerColor!");
        return FUNCTION_FAILURE;
    } else if ( header->cupsColorOrder != CUPS_ORDER_CHUNKED ) {
        log_error("Error", "Bad cupsColorOrder!");
        return FUNCTION_FAILURE;
    } else if (
        header->cupsColorSpace != CUPS_CSPACE_W &&
        header->cupsColorSpace != CUPS_CSPACE_RGB
    ) {
        log_error("Error", "Bad cupsColorSpace!");
        return FUNCTION_FAILURE;
    }

    /* 输出页面设置指令。 */
    fprintf(stderr, "PAGE %u %u %u %u\n", header->Margins[0], header->Margins[1], header->PageSize[0], header->PageSize[1]);
    fprintf(stderr, "RASTER %u %u %u\n", header->cupsWidth, header->cupsHeight, header->cupsNumColors);

    return FUNCTION_SUCCESS;
}

/*
 * OutputLine() - 输出一行的 raster 数据。
 */
static int                          /* 输出 - 1 成功，0 失败 */
output_line(
    cups_page_header2_t *header,    /* 输入 - 页头 */
    unsigned char       *line       /* 输入 - Raster 数据 */
) {

    return FUNCTION_SUCCESS;
}

/*
 * end_page() - 结束处理
 */

/*
 * SignalHandler() - 信号处理。
 */
static void SignalHandler(int sig) {
    CancelJob = 1;
}

