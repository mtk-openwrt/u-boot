// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <dm/ofnode.h>

#include "bootmenu_common.h"
#include "colored_print.h"
#include "mmc_helper.h"
#include "bl2_helper.h"
#include "fip_helper.h"
#include "verify_helper.h"

#define MMC_DEV_INDEX		CONFIG_MTK_BOOTMENU_MMC_DEV_INDEX

#define GPT_MAX_SIZE		(34 * 512)

int write_mmc_part(const char *partname, const void *data, size_t size,
		   bool verify)
{
	return mmc_write_part(MMC_DEV_INDEX, 0, partname, data, size, verify);
}

int read_mmc_part(const char *partname, void *data, size_t *size,
		  size_t max_size)
{
	u64 part_size;
	int ret;

	ret = mmc_read_part_size(MMC_DEV_INDEX, 0, partname, &part_size);
	if (ret)
		return -EINVAL;

	if (part_size > (u64)max_size)
		return -ENOBUFS;

	*size = (size_t)part_size;

	return mmc_read_part(MMC_DEV_INDEX, 0, partname, data, *size);
}

/******************************************************************************/

int generic_mmc_boot_image(void)
{
	return mmc_boot_image(MMC_DEV_INDEX);
}

int generic_emmc_write_bl2(void *priv, const struct data_part_entry *dpe,
			   const void *data, size_t size)
{
	int ret;

	ret = mmc_write_generic(MMC_DEV_INDEX, 1, 0, SZ_1M, data, size, true);
	if (!ret)
		mmc_setup_boot_options(MMC_DEV_INDEX);

	return ret;
}

int generic_sd_write_bl2(void *priv, const struct data_part_entry *dpe,
			 const void *data, size_t size)
{
	return write_mmc_part(PART_BL2_NAME, data, size, true);
}

int generic_mmc_write_bl2(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	if (mmc_is_sd(MMC_DEV_INDEX))
		return generic_sd_write_bl2(priv, dpe, data, size);

	return generic_emmc_write_bl2(priv, dpe, data, size);
}

int generic_mmc_write_fip_uda(void *priv, const struct data_part_entry *dpe,
			      const void *data, size_t size)
{
	return write_mmc_part(PART_FIP_NAME, data, size, true);
}

int generic_mmc_update_bl31(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size)
{
	size_t fip_part_size;
	size_t new_fip_size;
	size_t buf_size;
	void *buf;
	int ret;

	ret = get_fip_buffer(FIP_READ_BUFFER, &buf, &buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP buffer failed (%d) ***", ret);
		return -ENOBUFS;
	}

	ret = read_mmc_part("fip", buf, &fip_part_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP read failed (%d) ***", ret);
		return -EBADMSG;
	}

	if (size > fip_part_size) {
		cprintln(ERROR, "*** Invalid image size (0x%lx) ***", size);
		return -EINVAL;
	}

	ret = fip_update_bl31_data(data, size, buf, fip_part_size,
				   &new_fip_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP update u-boot failed (%d) ***", ret);
		return -EBADMSG;
	}

	return write_mmc_part("fip", buf, new_fip_size, true);
}

int generic_mmc_update_bl33(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size)
{
	size_t fip_part_size;
	size_t new_fip_size;
	size_t buf_size;
	void *buf;
	int ret;

	ret = get_fip_buffer(FIP_READ_BUFFER, &buf, &buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP buffer failed (%d) ***", ret);
		return -ENOBUFS;
	}

	ret = read_mmc_part("fip", buf, &fip_part_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP read failed (%d) ***", ret);
		return -EBADMSG;
	}

	if (size > fip_part_size) {
		cprintln(ERROR, "*** Invalid image size (0x%lx) ***", size);
		return -EINVAL;
	}

	ret = fip_update_uboot_data(data, size, buf, fip_part_size,
				    &new_fip_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP update u-boot failed (%d) ***", ret);
		return -EBADMSG;
	}

	return write_mmc_part("fip", buf, new_fip_size, true);
}

int generic_mmc_write_fw(void *priv, const struct data_part_entry *dpe,
			 const void *data, size_t size)
{
	return mmc_upgrade_image(MMC_DEV_INDEX, data, size);
}

#ifdef CONFIG_MTK_DUAL_BOOT_EMERG_IMAGE
int generic_mmc_write_emerg_fw(void *priv, const struct data_part_entry *dpe,
			       const void *data, size_t size)
{
#ifdef CONFIG_MTK_DUAL_BOOT_ITB_IMAGE
	return mmc_upgrade_image_cust(MMC_DEV_INDEX, data, size,
				      CONFIG_MTK_DUAL_BOOT_EMERG_FIRMWARE_NAME,
				      NULL);
#else
	return mmc_upgrade_image_cust(MMC_DEV_INDEX, data, size,
				      CONFIG_MTK_DUAL_BOOT_EMERG_IMAGE_KERNEL_NAME,
				      CONFIG_MTK_DUAL_BOOT_EMERG_IMAGE_ROOTFS_NAME);
#endif
}
#endif

int generic_mmc_write_simg(void *priv, const struct data_part_entry *dpe,
			   const void *data, size_t size)
{
	int ret;

	/* Write data without GPT */
	ret = mmc_write_generic(MMC_DEV_INDEX, 0, GPT_MAX_SIZE, 0,
				data + GPT_MAX_SIZE, size - GPT_MAX_SIZE, true);
	if (ret)
		return ret;

	/* Adjust and write GPT */
	return mmc_write_gpt(MMC_DEV_INDEX, 0, GPT_MAX_SIZE, data,
			     GPT_MAX_SIZE);
}

int generic_mmc_write_gpt(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	return mmc_write_gpt(MMC_DEV_INDEX, 0, GPT_MAX_SIZE, data, size);
}

int generic_mmc_validate_fw(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size)
{
	struct owrt_image_info ii;
	bool rc, verify_rootfs;

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_IMAGE_VERIFY)) {
		verify_rootfs = CONFIG_IS_ENABLED(MTK_UPGRADE_IMAGE_ROOTFS_VERIFY);

		rc = verify_image_ram(data, size, SZ_512K, verify_rootfs, &ii,
				      NULL, NULL);
		if (!rc) {
			cprintln(ERROR, "*** Firmware integrity verification failed ***");
			return -EBADMSG;
		}
	}

	return 0;
}

#ifdef CONFIG_ENV_SIZE
int generic_mmc_erase_env(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	const char *env_part_name =
		ofnode_conf_read_str("u-boot,mmc-env-partition");

	if (!env_part_name)
		return 0;

	return mmc_erase_env_part(MMC_DEV_INDEX, 0, env_part_name, 0);
}
#endif
