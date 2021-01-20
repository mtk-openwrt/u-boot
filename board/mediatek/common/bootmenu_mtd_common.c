// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <mtd.h>
#include <ubi_uboot.h>

#include "bootmenu_common.h"
#include "colored_print.h"
#include "mtd_helper.h"
#include "bl2_helper.h"
#include "fip_helper.h"
#include "verify_helper.h"

#ifdef CONFIG_ENV_IS_IN_UBI
#if (CONFIG_ENV_UBI_VID_OFFSET == 0)
 #define UBI_VID_OFFSET NULL
#else
 #define UBI_VID_OFFSET QUOTE(CONFIG_ENV_UBI_VID_OFFSET)
#endif
#endif /* CONFIG_ENV_IS_IN_UBI */

struct mtd_info *get_mtd_part(const char *partname)
{
	struct mtd_info *mtd;

	gen_mtd_probe_devices();

	if (partname)
		mtd = get_mtd_device_nm(partname);
	else
		mtd = get_mtd_device(NULL, 0);

	if (IS_ERR(mtd)) {
		cprintln(ERROR, "*** MTD partition '%s' not found! ***",
			 partname);
	}

	return mtd;
}

int read_mtd_part(const char *partname, void *data, size_t *size,
		  size_t max_size)
{
	struct mtd_info *mtd;
	u64 part_size;
	int ret;

	mtd = get_mtd_part(partname);
	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	part_size = mtd->size;

	if (part_size > (u64)max_size) {
		ret = -ENOBUFS;
		goto err;
	}

	*size = (size_t)part_size;

	ret = mtd_read_skip_bad(mtd, 0, mtd->size, mtd->size, NULL, data);

err:
	put_mtd_device(mtd);

	return ret;
}

int write_mtd_part(const char *partname, const void *data, size_t size,
		   bool verify)
{
	struct mtd_info *mtd;
	int ret;

	mtd = get_mtd_part(partname);
	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	ret = mtd_update_generic(mtd, data, size, verify);

	put_mtd_device(mtd);

	return ret;
}

/******************************************************************************/

int generic_mtd_boot_image(void)
{
	return mtd_boot_image();
}

int generic_mtd_write_bl2(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	return write_mtd_part(PART_BL2_NAME, data, size, true);
}

int generic_mtd_write_fip(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	return write_mtd_part(PART_FIP_NAME, data, size, true);
}

int generic_mtd_update_bl31(void *priv, const struct data_part_entry *dpe,
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

	ret = read_mtd_part(PART_FIP_NAME, buf, &fip_part_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP read failed (%d) ***", ret);
		return -EBADMSG;
	}
	ret = fip_update_bl31_data(data, size, buf, fip_part_size,
				   &new_fip_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP update u-boot failed (%d) ***", ret);
		return -EBADMSG;
	}

	return write_mtd_part(PART_FIP_NAME, buf, new_fip_size, true);
}

int generic_mtd_update_bl33(void *priv, const struct data_part_entry *dpe,
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

	ret = read_mtd_part(PART_FIP_NAME, buf, &fip_part_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP read failed (%d) ***", ret);
		return -EBADMSG;
	}

	ret = fip_update_uboot_data(data, size, buf, fip_part_size,
				    &new_fip_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP update u-boot failed (%d) ***", ret);
		return -EBADMSG;
	}

	return write_mtd_part(PART_FIP_NAME, buf, new_fip_size, true);
}

int generic_mtd_write_fw(void *priv, const struct data_part_entry *dpe,
			 const void *data, size_t size)
{
	int ret = 0;

	ret = mtd_upgrade_image(data, size);
	if (ret)
		cprintln(ERROR, "*** Image not supported! ***");

	return ret;
}

int generic_mtd_write_simg(void *priv, const struct data_part_entry *dpe,
			   const void *data, size_t size)
{
	struct mtd_info *mtd;
	int ret;

#ifdef CONFIG_ENABLE_NAND_NMBM
	mtd = get_mtd_part("nmbm0");
#else
	mtd = get_mtd_part(NULL);
#endif

	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	ret = mtd_erase_skip_bad(mtd, 0, mtd->size, mtd->size, NULL, NULL,
				 false);
	if (ret) {
		put_mtd_device(mtd);
		return ret;
	}

	ret = mtd_write_skip_bad(mtd, 0, size, mtd->size, NULL, data, true);

	put_mtd_device(mtd);

	return ret;
}

int generic_mtd_validate_fw(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size)
{
	struct owrt_image_info ii;
	bool rc, verify_rootfs;
	struct mtd_info *mtd;

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_IMAGE_VERIFY)) {
		mtd = get_mtd_part(NULL);
		if (IS_ERR(mtd))
			return -PTR_ERR(mtd);

		put_mtd_device(mtd);

		verify_rootfs = IS_ENABLED(CONFIG_MTK_UPGRADE_IMAGE_ROOTFS_VERIFY);

		rc = verify_image_ram(data, size, mtd->erasesize,
				      verify_rootfs, &ii, NULL, NULL);
		if (!rc) {
			cprintln(ERROR, "*** Firmware integrity verification failed ***");
			return -EBADMSG;
		}
	}

	return 0;
}

#ifdef CONFIG_ENV_IS_IN_MTD
int generic_mtd_erase_env_part(void *priv, const struct data_part_entry *dpe,
			       const void *data, size_t size)
{
	struct mtd_info *mtd;
	int ret;

	mtd = get_mtd_part(CONFIG_ENV_MTD_NAME);
	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	ret = mtd_erase_skip_bad(mtd, CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE,
				 mtd->size, NULL, "environment", false);

	put_mtd_device(mtd);

	return ret;
}
#endif

#ifdef CONFIG_ENV_IS_IN_UBI
int generic_mtd_erase_env_ubi(void *priv, const struct data_part_entry *dpe,
			      const void *data, size_t size)
{
	if (ubi_part(CONFIG_ENV_UBI_PART, UBI_VID_OFFSET))
		return -EIO;

	ubi_remove_vol(CONFIG_ENV_UBI_VOLUME);

#ifdef CONFIG_SYS_REDUNDAND_ENVIRONMENT
	ubi_remove_vol(CONFIG_ENV_UBI_VOLUME_REDUND);
#endif

	ubi_exit();

	return 0;
}
#endif
