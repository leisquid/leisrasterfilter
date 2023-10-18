/*
 * sample.h - a source code file of Leisrasterfilter
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

#ifndef __LEISRASTERFILTER_SAMPLE_H
#define __LEISRASTERFILTER_SAMPLE_H

#include <cups/cups.h>
#include <cups/ppd.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

/* 
 * 任务数据。
 */

typedef struct {
    int             job_id;         /* 任务 id */
    const char      *user,          /* 提交任务的用户 */
                    *title;         /* 任务标题 */
    int             num_options;    /* 命令行选项个数 */
    cups_option_t   *options;       /* 命令行选项 */
} job_data_t;

extern int          GetStatus(ppd_file_t *ppd, double timeout);
extern ppd_file_t   *Initialize(int argc, char *argv[], job_data_t *job);
extern void         LogMessage(char *prefix, char *content);

#endif
