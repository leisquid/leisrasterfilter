/*
 * rastertosample.c - a source code file of Leisrasterfilter
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

#include "sample.h"
#include <cups/raster.h>
#include <signal.h>

static int  CancelJob = 0;          /* 设为 1 时取消当前任务 */

static int  Setup(ppd_file_t *ppd, job_data_t *job);
static int  StartPage(ppd_file_t *ppd, job_data_t *job, cups_page_header2_t *header);
static int  OutputLine(ppd_file_t *ppd, cups_page_header2_t *header, unsigned char *line);
static int  EndPage(ppd_file_t *ppd, job_data_t *job, cups_page_header2_t *header);
static int  Shutdown(ppd_file_t *ppd, job_data_t *job);
static void SignalHandler(int sig);


int main(int argc, char *argv[]) {
    ppd_file_t          *ppd;       /* 用于打印机的 PPD 文件 */
    job_data_t          job;        /* Job data */
    int                 page = 0;   /* 当前页数 */
    int                 fd;         /* raster 数据的文件描述符 */
    cups_raster_t       *ras;       /* raster 流 */
    cups_page_header2_t header;     /* 当前页头数据 */
    unsigned            y;          /* 当前行 */
    unsigned char       *line;      /* 行缓冲 */

    // sleep(30);      // sleep to make it attachable by GDB

    /* 常规驱动初始化操作。 */
    if ( ( ppd = Initialize(argc, argv, &job) ) == NULL ) {
        LogMessage("WARNING", "Initialization failed");
        // return EXIT_FAILURE;
    }

    /* 注册一个信号处理器。 */
    signal(SIGTERM, SignalHandler);

    /* 准备打印任务。 */
    if ( ! Setup(ppd, &job) ) {
        return EXIT_FAILURE;
    }

    /* 打开 raster 流。 */
    if ( argc >= 6 ) {
        if ( ( fd = open(argv[6], O_RDONLY) ) == -1 ) {
            LogMessage("ERROR", "Unable to open raster file");
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
        fprintf(stderr, "DEBUG: cupsBytesPerLine=%u\n", header.cupsBytesPerLine);

        if ( ( line = malloc(header.cupsBytesPerLine) ) == NULL ) {
            LogMessage("ERROR", "Unable to allocate line memory!");
            break;
        }

        /* Let the scheduler and user know we are printing a page... */
        page ++;

        fprintf(stderr, "PAGE: %d %d\n", page, header.NumCopies);
        LogMessage("INFO", "Starting page");

        if ( !StartPage(ppd, &job, &header) ) {
            break;
        }

        /* 打印页面上的每一行。 */
        for ( y = 0; y < header.cupsHeight; y ++ ) {
            /* 检查是否有任务取消。 */
            if ( CancelJob ) {
                break;
            }

            /* 显示进度。 */
            if ( ( y & 128 ) == 0 ) {
                fprintf(stdout, "++ Printing page %d, %.0f%% completed\n", page, (100.0 * y / header.cupsHeight));
                puts("LEVELS");
                fflush(stdout);
            }

            /* 根据设备检查状态信息。 */
            GetStatus(ppd, 0.0);

            /* 读写每一行。 */
            if ( cupsRasterReadPixels(ras, line, header.cupsBytesPerLine) > 0 ) {
                if ( ! OutputLine(ppd, &header, line) ) {
                    break;
                }
            } else {
                break;
            }
        }

        /* 释放行缓存。 */
        free(line);

        /* 显示进度并结束当前页。 */
        LogMessage("INFO", "Finished page");

        if ( ! EndPage(ppd, &job, &header) ) {
            break;
        }
    }

    /* 检查打印机状态。 */
    GetStatus(ppd, 1.0);

    /* 结束打印任务。 */
    Shutdown(ppd, &job);

    /* 显示最终状态。 */
    if ( page == 0 ) {
        LogMessage("ERROR", "No pages found!");
        return EXIT_FAILURE;
    } else {
        LogMessage("INFO", "Ready to print.");
        return EXIT_SUCCESS;
    }
}

/*
 * Setup() - 为打印任务设置打印机。
 */
static int              /* 输出 - 1 成功，0 失败 */
Setup(
    ppd_file_t  *ppd,   /* 输入 - 打印机用 PPD 文件 */
    job_data_t  *job    /* 输入 - 任务数据 */
) {
    /* 参数发送到打印机。 */
    puts("DOCUMENT");
    printf("AUTHOR %s\n", job->user);
    printf("TITLE %s\n", job->title);

    return 1;
}

/*
 * StartPage() - Start a page on the printer.
 */
static int                          /* 输出 - 1 成功，0 失败 */
StartPage(
    ppd_file_t *ppd,                /* 输入 - 打印机用 PPD 文件 */
    job_data_t *job,                /* 输入 - 任务数据 */
    cups_page_header2_t *header     /* 输入 - 页头 */
) {
    if (
        header->cupsBitsPerColor != 8 &&
        header->cupsBitsPerColor != 16
    ) {
        LogMessage("ERROR", "Bad cupsBitsPerColor");
        return 0;
    } else if ( header->cupsColorOrder != CUPS_ORDER_CHUNKED ) {
        LogMessage("ERROR", "Bad cupsColorOrder");
        return 0;
    } else if (
        header->cupsColorSpace != CUPS_CSPACE_W &&
        header->cupsColorSpace != CUPS_CSPACE_RGB
    ) {
        LogMessage("ERROR", "Bad cupsColorSpace");
        return 0;
    }

    /* 页面设置指令发送到打印机。 */
    printf("PAGE %u %u %u %u\n", header->Margins[0], header->Margins[1], header->PageSize[0], header->PageSize[1]);
    printf("RASTER %u %u %u\n", header->cupsWidth, header->cupsHeight, header->cupsNumColors);

    return 1;
}

/*
 * OutputLine() - 输出一行的 raster 数据。
 */
static int                          /* 输出 - 1 成功，0 失败 */
OutputLine(
    ppd_file_t          *ppd,       /* 输入 - 打印机用 PPD 文件 */
    cups_page_header2_t *header,    /* 输入 - 页头 */
    unsigned char       *line       /* 输入 - Raster 数据 */
) {
    /* 将一行 raster 数据发送到打印机。 */
    if ( header->cupsBitsPerColor == 8 ) {
        /* 将 8 位数据发送到打印机。实际上这里输出到标准输出了。 */
        printf("LINE %d\n", header->cupsBytesPerLine);
        return ( fwrite(line, 1, header->cupsBytesPerLine, stdout) == header->cupsBytesPerLine );
    } else {
        /*
         * 将 16 位数据发送到打印机。通常是要做抖动处理的，但是这里只要将 48 位 RGB
         * 数据转为 24 位。
         *
         * 这个公式：
         *
         *     (*pixel + 129) / 257
         *
         * 将 16 位像素值截断为近似的 8 位值 ("+ 129") 并从 16 位转为 8 位
         * (65535 / 255 = 257)。
         */

        unsigned short  *pixel;     /* 当前像素 */
        int             count;      /* 剩余计数 */

        printf("LINE %d\n", header->cupsBytesPerLine / 2);

        for (
            pixel = ( unsigned short * ) line, count = header->cupsBytesPerLine / 2;
            count > 0;
            count --, pixel ++
        ) {
            if ( putchar(( *pixel + 129 ) / 257) == EOF ) {
                return 0;
            }
        }

        return 1;
    }
}

/*
 * EndPage() - 结束打印机处理的当前页面。
 */
static int
EndPage(
    ppd_file_t *ppd,            /* I - PPD file for printer */
    job_data_t *job,            /* I - Job data */
    cups_page_header2_t *header /* I - Page header */
) {
    /* 向打印机（大嘘）发送结束页面指令。 */
    puts("ENDPAGE");
    return 1;
}

/*
 * Shutdown() - 结束打印机的当前任务。
 */
static int                  /* O - 1 on success, 0 on failure */
Shutdown(
    ppd_file_t *ppd,        /* I - PPD file for printer */
    job_data_t *job         /* I - Job data */
) {
    puts("ENDDOCUMENT");
    return 1;

}

/*
 * SignalHandler() - 信号处理。
 */
static void SignalHandler(int sig) {
    CancelJob = 1;
}
