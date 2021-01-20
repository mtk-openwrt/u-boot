/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2024 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _BOOTMENU_COMMON_H_
#define _BOOTMENU_COMMON_H_

#include <linux/kernel.h>
#include <linux/mtd/mtd.h>

#include "upgrade_helper.h"

#define PART_BL2_NAME		"bl2"
#define PART_FIP_NAME		"fip"

struct mtd_info *get_mtd_part(const char *partname);

int read_mtd_part(const char *partname, void *data, size_t *size,
		  size_t max_size);
int write_mtd_part(const char *partname, const void *data, size_t size,
		   bool verify);

/******************************************************************************/

int write_mmc_part(const char *partname, const void *data, size_t size,
		   bool verify);
int read_mmc_part(const char *partname, void *data, size_t *size,
		  size_t max_size);

/******************************************************************************/

int generic_mtd_boot_image(void);

int generic_mtd_write_bl2(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size);

int generic_mtd_write_fip(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size);

int generic_mtd_update_bl31(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size);

int generic_mtd_update_bl33(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size);

int generic_mtd_write_fw(void *priv, const struct data_part_entry *dpe,
			 const void *data, size_t size);

int generic_mtd_write_simg(void *priv, const struct data_part_entry *dpe,
			   const void *data, size_t size);

int generic_mtd_validate_fw(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size);

#ifdef CONFIG_ENV_IS_IN_MTD
int generic_mtd_erase_env_part(void *priv, const struct data_part_entry *dpe,
			       const void *data, size_t size);
#endif

#ifdef CONFIG_ENV_IS_IN_UBI
int generic_mtd_erase_env_ubi(void *priv, const struct data_part_entry *dpe,
			      const void *data, size_t size);
#endif

/******************************************************************************/

int generic_mmc_boot_image(void);

int generic_emmc_write_bl2(void *priv, const struct data_part_entry *dpe,
			   const void *data, size_t size);

int generic_sd_write_bl2(void *priv, const struct data_part_entry *dpe,
			 const void *data, size_t size);

int generic_mmc_write_bl2(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size);

int generic_mmc_write_fip_uda(void *priv, const struct data_part_entry *dpe,
			      const void *data, size_t size);

int generic_mmc_update_bl31(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size);

int generic_mmc_update_bl33(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size);

int generic_mmc_write_fw(void *priv, const struct data_part_entry *dpe,
			 const void *data, size_t size);

#ifdef CONFIG_MTK_DUAL_BOOT_EMERG_IMAGE
int generic_mmc_write_emerg_fw(void *priv, const struct data_part_entry *dpe,
			       const void *data, size_t size);
#endif

int generic_mmc_write_simg(void *priv, const struct data_part_entry *dpe,
			   const void *data, size_t size);

int generic_mmc_write_gpt(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size);

int generic_mmc_validate_fw(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size);

#ifdef CONFIG_ENV_SIZE
int generic_mmc_erase_env(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size);
#endif

/******************************************************************************/

int generic_validate_bl2(void *priv, const struct data_part_entry *dpe,
			 const void *data, size_t size);

int generic_validate_fip(void *priv, const struct data_part_entry *dpe,
			 const void *data, size_t size);

int generic_validate_bl31(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size);

int generic_validate_bl33(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size);

int generic_validate_simg(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size);

#endif /* _BOOTMENU_COMMON_H_ */
