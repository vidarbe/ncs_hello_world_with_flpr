/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * FLPR-side IPC demo (CONFIG_MULTITHREADING=n): same ipc_service API as
 * samples/subsys/ipc/ipc_service/icmsg/remote, using volatile flags instead of
 * kernel semaphores where the sample would use k_sem.
 */

#include "ipc_demo.h"
#include "ipc_demo_protocol.h"

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(ipc_demo_remote, LOG_LEVEL_INF);

static volatile uint32_t ep_bound_flag = 1;
static volatile uint32_t have_hello;
static struct ipc_demo_msg rx_hello;

static void ep_bound(void *priv)
{
	ep_bound_flag = 0;
	LOG_INF("IPC endpoint bound");
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	if (len < sizeof(struct ipc_demo_msg)) {
		return;
	}

	memcpy(&rx_hello, data, sizeof(rx_hello));

	if (rx_hello.magic != IPC_DEMO_MAGIC || rx_hello.kind != IPC_DEMO_KIND_HELLO) {
		return;
	}

	have_hello = 1;
}

static struct ipc_ept_cfg ep_cfg = {
	.cb =
		{
			.bound = ep_bound,
			.received = ep_recv,
		},
};

void ipc_demo_remote_main(void)
{
	const struct device *ipc = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	struct ipc_ept ep;
	int ret;

	printk("FLPR IPC demo (%s)\n", CONFIG_BOARD_TARGET);

	if (!device_is_ready(ipc)) {
		LOG_ERR("ipc0 not ready");
		return;
	}

	ret = ipc_service_open_instance(ipc);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("ipc_service_open_instance failed: %d", ret);
		return;
	}

	ret = ipc_service_register_endpoint(ipc, &ep, &ep_cfg);
	if (ret < 0) {
		LOG_ERR("ipc_service_register_endpoint failed: %d", ret);
		return;
	}

	while (ep_bound_flag != 0) {
		k_busy_wait(100);
	}

	while (have_hello == 0) {
		k_busy_wait(100);
	}

	struct ipc_demo_msg reply = {
		.magic = IPC_DEMO_MAGIC,
		.kind = IPC_DEMO_KIND_REPLY,
		.seq = rx_hello.seq + 1U,
	};
	strncpy(reply.payload, "hello-flpr", sizeof(reply.payload) - 1);

	for (;;) {
		ret = ipc_service_send(&ep, &reply, sizeof(reply));
		if (ret == (int)sizeof(reply)) {
			break;
		}
		if (ret == -ENOMEM) {
			k_busy_wait(50);
			continue;
		}
		LOG_ERR("ipc_service_send failed: %d", ret);
		return;
	}

	LOG_INF("Sent IPC reply seq=%u", reply.seq);
}
