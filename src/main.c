/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <hal/nrf_vpr.h>
#include "flpr_firmware.h"
#include <hal/nrf_spu.h>
#include <app_version.h>
#include <ram_pwrdn.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME app
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#if DT_NODE_EXISTS(DT_NODELABEL(cpuflpr_sram_code_data))
#define FLPR_SRAM_GLOBAL_ADDR (DT_REG_ADDR(DT_NODELABEL(cpuflpr_sram_code_data)))
#else
#error "DT node cpuflpr_sram_code_data not defined."
#endif

static void flpr_start(void)
{
	power_up_ram(FLPR_SRAM_GLOBAL_ADDR, FLPR_SRAM_GLOBAL_ADDR + FLPR_FIRMWARE_SIZE);

	LOG_INF("Copying %u bytes of FLPR firmware to SRAM @ 0x%08x ...", FLPR_FIRMWARE_SIZE,
		FLPR_SRAM_GLOBAL_ADDR);

	/* Copy the firmware blob into FLPR SRAM. */
	memcpy((uint8_t *)FLPR_SRAM_GLOBAL_ADDR, flpr_firmware, FLPR_FIRMWARE_SIZE);

	nrf_spu_periph_perm_secattr_set(NRF_SPU00, nrf_address_slave_get((uint32_t)NRF_VPR00),
					true);

	nrf_vpr_initpc_set(NRF_VPR00, FLPR_SRAM_GLOBAL_ADDR);
	nrf_vpr_cpurun_set(NRF_VPR00, true);
}

static void flpr_stop(void)
{
	/* Stop VPR */
	nrf_vpr_cpurun_set(NRF_VPR00, false);

	/* Reset the FLPR core */
	nrf_vpr_debugif_dmcontrol_mask_set(
		NRF_VPR00,
		(VPR_DEBUGIF_DMCONTROL_NDMRESET_Active << VPR_DEBUGIF_DMCONTROL_NDMRESET_Pos |
		 VPR_DEBUGIF_DMCONTROL_DMACTIVE_Enabled << VPR_DEBUGIF_DMCONTROL_DMACTIVE_Pos));
	nrf_vpr_debugif_dmcontrol_mask_set(
		NRF_VPR00,
		(VPR_DEBUGIF_DMCONTROL_NDMRESET_Inactive << VPR_DEBUGIF_DMCONTROL_NDMRESET_Pos |
		 VPR_DEBUGIF_DMCONTROL_DMACTIVE_Disabled << VPR_DEBUGIF_DMCONTROL_DMACTIVE_Pos));

	/* Power off RAM to reduce idle current */
	power_down_ram(FLPR_SRAM_GLOBAL_ADDR, (FLPR_SRAM_GLOBAL_ADDR + FLPR_FIRMWARE_SIZE));
}

int main(void)
{
	LOG_INF("Hello world from %s. Version: %s", CONFIG_BOARD_TARGET, APP_VERSION_STRING);
	flpr_start();
	k_msleep(1000);
	flpr_stop();
	return 0;
}
