// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: guan-gm.lin <guan-gm.lin@mediatek.com>
 */

#ifndef USE_HOSTCC
#include <malloc.h>
#include <linux/arm-smccc.h>
#include <linux/iopoll.h>
#include <misc.h>
#include <dm.h>
#include <log.h>
#include <misc.h>
#endif /* ifndef USE_HOSTCC */
#include <image.h>
#include <uboot_aes.h>

#ifndef USE_HOSTCC

#define MTK_SIP_FW_DEC_SET_IV			0xC2000580
#define MTK_SIP_FW_DEC_SET_SALT			0xC2000581
#define MTK_SIP_FW_DEC_IMAGE			0xC2000582

static int get_salt(const void *fit, unsigned char **salt, u32 *salt_len)
{
	int image_noffset;

	image_noffset = fdt_path_offset(fit, FIT_IMAGES_PATH);
	if (image_noffset < 0) {
		printf("Can't get found '/images'""\n");
		return -1;
	}

	*salt = (unsigned char*)fdt_getprop(fit, image_noffset, "salt", salt_len);
	if(*salt == NULL) {
		printf("Can't get salt\n");
		return -1;
	}

	return 0;
}

static int set_iv(unsigned char *iv, u32 iv_len)
{
	struct arm_smccc_res res;

	arm_smccc_smc(MTK_SIP_FW_DEC_SET_IV, (uintptr_t)iv, iv_len, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

static int set_salt(unsigned char *salt, u32 salt_len)
{
	struct arm_smccc_res res;

	arm_smccc_smc(MTK_SIP_FW_DEC_SET_SALT, (uintptr_t)salt, salt_len, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

static int set_buffer(unsigned char *cipher, u32 cipher_len,
		      unsigned char *plain, u32 plain_len)
{
	struct arm_smccc_res res;

	arm_smccc_smc(MTK_SIP_FW_DEC_IMAGE, (uintptr_t)cipher, cipher_len, (uintptr_t)plain, plain_len, 0, 0, 0, &res);

	return res.a0;

}

static int image_decrypt_via_smc(unsigned char *salt, u32 salt_len,
				   unsigned char *iv, u32 iv_len,
				   unsigned char *cipher, u32 cipher_len,
				   unsigned char *plain, u32 plain_len)
{
	int res;

	res = set_salt(salt, salt_len);
	if (res) {
		printf("Failed: set salt\n");
		goto out;
	}

	res = set_iv(iv, iv_len);
	if (res) {
		printf("Failed: set iv\n");
		goto out;
	}

	res = set_buffer(cipher, cipher_len,
			 plain, plain_len);
	if (res) {
		printf("Failed: set buffer\n");
		goto out;
	}

out:
	return res;
}
#endif /* ifndef USE_HOSTCC */

int mtk_image_aes_decrypt(struct image_cipher_info *info,
			  const void *cipher, size_t cipher_len,
			  void **data, size_t *size)
{
#ifndef USE_HOSTCC
	u32 salt_len, iv_len;
	unsigned char *salt, *iv;

	iv = (unsigned char*) info->iv;
	iv_len = info->cipher->iv_len;
	if (iv == NULL || iv_len == 0) {
		printf("iv is NULL or iv_len is 0\n");
		return -EINVAL;
	}

	if (get_salt(info->fit, &salt, &salt_len))
		return -EINVAL;

	/* use same buffer in cipher and plain*/
	*data = (void *)cipher;
	*size = info->size_unciphered;

	if(image_decrypt_via_smc(salt, salt_len, iv, iv_len,
			(unsigned char *)cipher, cipher_len, *data, cipher_len)) {
		printf("Failed: image decryption via SMC call\n");
		return -EINVAL;
	}

#endif /* ifndef USE_HOSTCC */
	return 0;
}
