## SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2022 MediaTek Incorporation. All Rights Reserved.
#
# Author: guan-gm.lin <guan-gm.lin@mediatek.com>
#

fdtput $1 /firmware/optee -cp
fdtput $1 /firmware/optee -ts compatible "linaro,optee-tz"
fdtput $1 /firmware/optee -ts method "smc"
fdtput $1 /firmware/optee -ts status "okay"

fdtput $1 /cipher/key-$2-$3_key -cp
fdtput $1 /cipher/key-$2-$3_key -tx key-len 0x20
fdtput $1 /cipher/key-$2-$3_key -tx key 0x0
