/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2024 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * OpenWrt rootdisk settings
 */
#ifndef _ROOTDISK_H_
#define _ROOTDISK_H_

int rootdisk_set_fitblk_rootfs(void *fdt, const char *block_part);

#endif /* _ROOTDISK_H_ */
