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
 *
 * 此文件是 Leisrasterfilter 的一部分。
 * Leisrasterfilter 是自由软件：您可以在遵照自由软件基金会发布的「GNU Affero 通用
 * 公共许可证」（第 3 版或者更新版本皆可）的前提下对其进行转载或者修改。
 * 发布 Leisrasterfilter 的初衷是希望它能有一定的用处，但是并不为销售或特定用途等
 * 情况做出任何担保。参见「GNU Affero 通用公共许可证」。
 * 您应该随 Leisrasterfilter 获得了一份「GNU Affero 通用公共许可证」的副本。如果
 * 没有，请看 <https://www.gnu.org/licenses/agpl-3.0.txt>。
 */

#include "bitmap.h"
#include <cups/raster.h>
#include <signal.h>

static int  CancelJob = 0;          /* 设为 1 时取消当前任务 */

static void SignalHandler(int sig);
static int setup(bitmap_job_data_t *job);
static int start_page(bitmap_job_data_t *job, cups_page_header2_t *header);
static int output_line(cups_page_header2_t *header, unsigned char *line, void *output_stream);
static int end_page(bitmap_job_data_t *job, cups_page_header2_t *header);
static int rtd_shutdown(bitmap_job_data_t *job);

/*
 * main() - 程序主入口。
 */
int                                     /* 输出 - 0 成功，1 失败 */
main(
    int argc,                           /* 输入 - 命令行参数个数。 */
    char *argv[]                        /* 输入 - 命令行参数内容。 */
) {
    bitmap_job_data_t   job;            /* 任务数据 */
    int                 page = 0;       /* 当前页数 */
    int                 fd;             /* raster 数据的文件描述符 */
    cups_raster_t       *ras;           /* raster 流 */
    cups_page_header2_t header;         /* 当前页头数据 */
    unsigned            y;              /* 当前行 */
    unsigned char       *line;          /* 行缓冲 */

    void                *buffer;        /* 像素阵缓冲 */
    FILE                *fp;            /* 输出文件 */
    char                filename[256];  /* 输出文件名 */

    bitmap_file_header  file_header;    /* bitmap 文件头部 */
    bitmap_info_header  info_header;    /* bitmap 位图头部 */

    // sleep(30);      /* sleep to make it attachable by GDB */

    /* 初始化操作。 */
    if ( ( init_job(argc, argv, &job) ) == FUNCTION_FAILURE ) {
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
            return EXIT_FAILURE;
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

        /* 分配页缓冲内存和行内存。 */
        if ( (
            buffer = (bitmap_24bit_pixel *) malloc(
                        sizeof(bitmap_24bit_pixel)
                        * header.cupsWidth
                        * header.cupsHeight
            ) ) == NULL
        ) {
            log_error("Error", "Unable to allocate page buffer!");
            break;
        }
        if ( (
            line = (unsigned char *) malloc(
                        header.cupsBytesPerLine
            ) ) == NULL ) {
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
                fprintf(stderr, "[--] Printing page %d, %.0f%% completed\n", page, (100.0 * y / header.cupsHeight));
                fflush(stderr);
            }

            /* 读写每一行。 */
            if ( cupsRasterReadPixels(ras, line, header.cupsBytesPerLine) > 0 ) {
                if ( ! output_line(&header, line, buffer) ) {
                    break;
                }
            } else {
                break;
            }
        }

        /*
         * 输出 bitmap 文件。
         */
        /* 对像素阵做上下反转处理。 */
        pixel_24bit_matrix_upsidedown(buffer, header.cupsWidth, header.cupsHeight);
        init_24bit_header(
            &file_header,
            &info_header,
            header.cupsWidth,
            header.cupsHeight
        );
        /* 生成文件名。 */
        fprintf(filename, "./output/%05d.bmp", page);
        /* 打开文件。 */
        fp = fopen(filename, "wb");
        if ( bitmap_24bit_write(file_header, info_header, buffer, fp) != FUNCTION_SUCCESS ) {
            log_error("ERROR", "File writing failure!");
        }
        fclose(fp);

        /* 释放内存。 */
        free(buffer);
        free(line);

        /* 显示进度并结束当前页。 */
        log_debug("Info", "Finishing page");

        if ( ! end_page(&job, &header) ) {
            break;
        }
    }

    /* 结束打印任务。 */
    rtd_shutdown(&job);

    /* 显示最终状态。 */
    if (page == 0) {
        log_error("Error", "No pages found!");
        return EXIT_FAILURE;
    } else {
        log_debug("Info", "Ready to print");
        return EXIT_SUCCESS;
    }
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
static int                              /* 输出 - 1 成功，0 失败 */
output_line(
    cups_page_header2_t *header,        /* 输入 - 页头 */
    unsigned char       *line,          /* 输入 - Raster 数据 */
    void                *output_stream  /* 输入 - 待写入的流指针 */
) {
    unsigned            num_pixels = header->cupsWidth;
    unsigned            pixels_count = 0;
    unsigned            line_repeating_count = 0,
                        run_length_count = 0;
    uint8_t             byte_buffer;
    uint8_t             pixel_buffer[3];
    uint16_t            pixel_48bit_buffer[3];
    bitmap_24bit_pixel  pixel;
    bitmap_24bit_pixel  *line_buffer,
                        *line_buffer_start_ptr;

    /* 给行缓冲分配内存。 */
    line_buffer = (bitmap_24bit_pixel *) malloc(sizeof(pixel) * num_pixels);
    /* 记下行缓冲起始位置的地址。 */
    line_buffer_start_ptr = line_buffer;

    if ( header->cupsBitsPerColor == 8 ) {  /* 色深为 8 位的情况 */
        /* 读行重复次数。 */
        fread(&byte_buffer, sizeof(byte_buffer), 1, line);
        line_repeating_count = byte_buffer + 1;
        /* 读写一个行的数据。 */
        do {
            /* 读游离计数。 */
            fread(&byte_buffer, sizeof(byte_buffer), 1, line);
            run_length_count = byte_buffer;
            /* 判断游离编码模式还是点连续写入模式。 */
            if (run_length_count >= 0 && run_length_count < 128) {
                /* 游离编码模式，确定重复像素个数。 */
                run_length_count ++;
                /* 读一个像素。 */
                fread(&pixel_buffer, sizeof(pixel_buffer), 1, line);
                /* 将读到的 raster 像素转为 bitmap 像素。 */
                set_24bit_pixel_color(
                    &pixel,
                    pixel_buffer[0],
                    pixel_buffer[1],
                    pixel_buffer[2]
                );
                /* 写入重复像素。 */
                while (run_length_count > 0) {
                    /* 写这个像素。 */
                    fwrite(&pixel, sizeof(pixel), 1, line_buffer);
                    run_length_count --;
                    pixels_count ++;
                }
            } else {
                /* 连续写入模式，确定连续像素个数。 */
                run_length_count = 257 - run_length_count;
                /* 写入连续像素。 */
                while (run_length_count > 0) {
                    /* 读一个像素。 */
                    fread(&pixel_buffer, sizeof(pixel_buffer), 1, line);
                    /* 将读到的 raster 像素转为 bitmap 像素。 */
                    set_24bit_pixel_color(
                        &pixel,
                        pixel_buffer[0],
                        pixel_buffer[1],
                        pixel_buffer[2]
                    );
                    /* 写这个像素。 */
                    fwrite(&pixel, sizeof(pixel), 1, line_buffer);
                    run_length_count --;
                    pixels_count ++;
                }
            }
        } while ( pixels_count < num_pixels );
        /* 一行读完了，开始向流写入数据。 */
        while ( line_repeating_count > 0 ) {
            line_buffer = line_buffer_start_ptr;    /* 行缓冲指针回到开头 */
            fwrite(line_buffer, sizeof(pixel) * num_pixels, 1, output_stream);
        }
    } else {
        /*
         * 假设其他情况都是 16 位色深。通常是要做抖动处理的，但是这里只将 48 位 RGB
         * 数据转为 24 位。
         * 
         * 这个公式：
         *     (*pixel + 129) / 257
         * 将 16 位像素值截断为近似的 8 位值 ("+ 129") 并从 16 位转为 8 位
         * (65535 / 255 = 257)。
         * 
         * 当然，游离计数还是 8 位的。
         */

        /* 读行重复次数。 */
        fread(&byte_buffer, sizeof(byte_buffer), 1, line);
        line_repeating_count = byte_buffer + 1;
        /* 读写一个行的数据。 */
        do {
            /* 读游离计数。 */
            fread(&byte_buffer, sizeof(byte_buffer), 1, line);
            run_length_count = byte_buffer;
            /* 判断游离编码模式还是点连续写入模式。 */
            if (run_length_count >= 0 && run_length_count < 128) {
                /* 游离编码模式，确定重复像素个数。 */
                run_length_count ++;
                /* 读一个 48 位像素。 */
                fread(&pixel_48bit_buffer, sizeof(pixel_48bit_buffer), 1, line);
                /* 将读到的 48 位 raster 像素转为 bitmap 像素。 */
                set_24bit_pixel_color(
                    &pixel,
                    ( ( pixel_48bit_buffer[0] + 129) / 257 ),
                    ( ( pixel_48bit_buffer[1] + 129) / 257 ),
                    ( ( pixel_48bit_buffer[2] + 129) / 257 )
                );
                /* 写入重复像素。 */
                while (run_length_count > 0) {
                    /* 写这个像素。 */
                    fwrite(&pixel, sizeof(pixel), 1, line_buffer);
                    run_length_count --;
                    pixels_count ++;
                }
            } else {
                /* 连续写入模式，确定连续像素个数。 */
                run_length_count = 257 - run_length_count;
                /* 写入连续像素。 */
                while (run_length_count > 0) {
                    /* 读一个 48 位像素。 */
                    fread(&pixel_48bit_buffer, sizeof(pixel_48bit_buffer), 1, line);
                    /* 将读到的 48 位 raster 像素转为 bitmap 像素。 */
                    set_24bit_pixel_color(
                        &pixel,
                        ( ( pixel_48bit_buffer[0] + 129) / 257 ),
                        ( ( pixel_48bit_buffer[1] + 129) / 257 ),
                        ( ( pixel_48bit_buffer[2] + 129) / 257 )
                    );
                    /* 写这个像素。 */
                    fwrite(&pixel, sizeof(pixel), 1, line_buffer);
                    run_length_count --;
                    pixels_count ++;
                }
            }
        } while ( pixels_count < num_pixels );
        /* 一行读完了，开始向流写入数据。 */
        while ( line_repeating_count > 0 ) {
            line_buffer = line_buffer_start_ptr;    /* 行缓冲指针回到开头 */
            fwrite(line_buffer, sizeof(pixel) * num_pixels, 1, output_stream);
            line_repeating_count --;
        }
    }
    /* 释放行缓冲。 */
    free(line_buffer);

    return FUNCTION_SUCCESS;
}

/*
 * end_page() - 结束处理当前页面。
 */
static int
end_page(                       /* 输出 - 1 成功，0 失败 */
    bitmap_job_data_t   *job,   /* 输入 - 任务数据 */
    cups_page_header2_t *header /* 输入 - 页头 */
) {
    fprintf(stderr, "END_OF_PAGE");
    return FUNCTION_SUCCESS;
}

/*
 * shutdown() - 结束当前任务。
 */
static int rtd_shutdown(            /* 输出 - 1 成功，0 失败 */
    bitmap_job_data_t   *job    /* 输入 - 任务数据 */
) {
    fprintf(stderr, "END_OF_DOCUMENT");
    return FUNCTION_SUCCESS;
}

/*
 * SignalHandler() - 信号处理。
 */
static void SignalHandler(int sig) {
    CancelJob = 1;
}
