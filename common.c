/*
 * common.c - a source code file of Leisrasterfilter
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

#include <stdarg.h>
#include "sample.h"			/* Common sample driver header */

/*
 * GetStatus() - 读取状态信息。
 */
int                     /* 输出 - 1 成功, 0 失败 */
GetStatus(
    ppd_file_t *ppd,    /* I - PPD file for printer */
    double     timeout  /* I - Timeout in seconds */
) {
    char        buffer[1024],   /* Buffer for back-channel data */
                *start,         /* Start of line */
                *end;           /* End of line */
    ssize_t     bytes;          /* 读取字节数 */
    static int  last_levels[4] = { -1, -1, -1, -1 };

    if ( timeout > 0.0 ) {
        /* Send a "get levels" command to the printer... */
        puts("LEVELS");
        fflush(stdout);
    }

    /* Read back-channel data from the backend with no timeout. */
    // if ( (
    //     bytes = cupsBackChannelRead(
    //         buffer, sizeof(buffer) - 1, timeout
    //     ) ) <= 0
    // ) {
    //     /* No data... */

    //     return ( timeout == 0.0 ? 1 : 0 );
    // }

    strcpy(buffer, "IL999,999,999,999\nOK\n");
    bytes = sizeof(buffer);

    /* 以 '\0' 作为 buffer 的终止符。 */
    buffer[bytes] = '\0';

    /*
     * 解析 back-channel 数据。对于我们的这个假想的设备，
     * 它自己在一行数据中返回以下字符串之一：
     *
     * ILnnn,nnn,nnn,nnn      （墨水量）
     * OP                     （缺纸）
     * LP                     （纸张不足）
     * OK                     （正常）
     *
     * Then we send ATTR: and STATE: messages to the scheduler.  See:
     *
     *     http://localhost:631/help/api-filter.html
     */
    for ( start = buffer; *start; start = end ) {
        /* Find the end of the current line... */

        if ( ( end = strchr(start, '\n') ) != NULL ) {
            *end++ = '\0';
        } else {
            end = start + strlen(start);
        }

        /* 解析这一行。 */
        if ( !strncmp(start, "IL", 2) ) {
            /* 收集墨水量信息。 */
            int	levels[4];      /* 墨水量 */

            if ( sscanf(
                start,
                "IL%d,%d,%d,%d",
                levels + 0,
                levels + 1,
                levels + 2,
                levels + 3
            ) != 4 ) {
                return ( 0 );   /* 问题行 */
            }

            /*
             * Only report levels if they have changed...
             */

            if ( levels[0] == last_levels[0] &&
                levels[1] == last_levels[1] &&
                levels[2] == last_levels[2] &&
                levels[3] == last_levels[3] )
                continue;

            /*
             * Write an ATTR: message to stderr...
             */

            fprintf(stderr,
                "ATTR: marker-colors=#00ffff,#ff00ff,#ffff00,#000000 "
                "marker-levels=%d,%d,%d,%d "
                "marker-names=Cyan,Magenta,Yellow,Black "
                "marker-types=ink,ink,ink,ink\n",
                levels[0], levels[1], levels[2], levels[3]);

            if ( levels[0] < 5 && last_levels[0] >= 5 )
                fputs("STATE: +com.sample-cyan-error\n", stderr);
            else if ( levels[0] >= 5 && last_levels[0] < 5 )
                fputs("STATE: -com.sample-cyan-error\n", stderr);

            if ( levels[1] < 5 && last_levels[1] >= 5 )
                fputs("STATE: +com.sample-magenta-error\n", stderr);
            else if ( levels[1] >= 5 && last_levels[1] < 5 )
                fputs("STATE: -com.sample-magenta-error\n", stderr);

            if ( levels[2] < 5 && last_levels[2] >= 5 )
                fputs("STATE: +com.sample-yellow-error\n", stderr);
            else if ( levels[2] >= 5 && last_levels[2] < 5 )
                fputs("STATE: -com.sample-yellow-error\n", stderr);

            if ( levels[3] < 5 && last_levels[3] >= 5 )
                fputs("STATE: +com.sample-black-error\n", stderr);
            else if ( levels[3] >= 5 && last_levels[3] < 5 )
                fputs("STATE: -com.sample-black-error\n", stderr);

            last_levels[0] = levels[0];
            last_levels[1] = levels[1];
            last_levels[2] = levels[2];
            last_levels[3] = levels[3];
        } else if ( !strcmp(start, "OP") ) {
            /*
             * Write out-of-paper STATE: messages to stderr...
             */

            fputs("STATE: -media-low-report\n", stderr);
            fputs("STATE: +media-empty-warning\n", stderr);
        } else if ( !strcmp(start, "LP") ) {
            /*
             * Write low-paper STATE: messages to stderr...
             */

            fputs("STATE: -media-empty-warning\n", stderr);
            fputs("STATE: +media-low-report\n", stderr);
        } else if ( !strcmp(start, "OK") ) {
            /*
             * Write no-error STATE: messages to stderr...
             */

            fputs("STATE: -media-empty-warning\n", stderr);
            fputs("STATE: -media-low-report\n", stderr);
        } else {
            fprintf(stderr, "DEBUG: Unknown status \"%s\"!\n", start);
            return  0 ;
        }
    }

    /* Return with no errors. */
    return 1;
}


/*
 * Initialize() - 打开 PPD 文件并分析（parse）选项。
 */
ppd_file_t                  /* 输出 - 用于打印机的 PPD 文件 */
*Initialize(
    int         argc,
    char        *argv[],
    job_data_t  *job        /* 输入 - 任务数据 */
) {
    ppd_file_t  *ppd;       /* 用于打印机的 PPD 文件 */
    int i;

    /* 检查命令行参数个数。
     * if ( argc < 6 || argc > 7 )
     */
    if ( argc != 7 ) {
        LogMessage("ERROR", "Arguments wrong");
        fprintf(stderr, "argc = %d\n", argc);
        for ( i = 0; i < argc; i ++ ) {
            fprintf(stderr, "argv[%d] = %s\n", i, argv[i]);
        }
        //                    0  1    2     3   4 5  6
        fprintf(stderr, "用法：%s 任务 用户名 标题 选项 [文件名]\n", argv[0]);
        return NULL;
    }

    /* 有个本地化设置？ */
    // SetLocale();

    /* 解析选项。 */
    job->job_id = atoi(argv[1]);
    job->user = argv[2];
    job->title = argv[3];
    job->num_options = cupsParseOptions(argv[5], 0, &( job->options ));

    /* 打开 PPD 文件。 */
    if ( ( ppd = ppdOpenFile(getenv("PPD")) ) != NULL ) {
        /*
         * Mark the options for the job...
         */
        ppdMarkDefaults(ppd);
        cupsMarkOptions(ppd, job->num_options, job->options);
    } else {
        LogMessage("WARNING", "Unable to open PPD file");
    }

    return ppd;
}

void LogMessage(char *prefix, char *content) {
    fprintf(stderr, "-------- %s -------- : %s\n", prefix, content);
}
