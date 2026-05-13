/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host-side IPC demo modeled on Zephyr samples/subsys/ipc/ipc_service/icmsg.
 */

#include "ipc_demo.h"
#include "ipc_demo_protocol.h"

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ipc_demo_host, LOG_LEVEL_INF);

static K_SEM_DEFINE(ep_bound_sem, 0, 1);
static K_SEM_DEFINE(reply_sem, 0, 1);
static struct ipc_demo_msg last_reply;

static void ep_bound(void *priv)
{
	k_sem_give(&ep_bound_sem);
	LOG_INF("IPC endpoint bound");
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	if (len < sizeof(struct ipc_demo_msg)) {
		return;
	}

	memcpy(&last_reply, data, sizeof(last_reply));

	if (last_reply.magic != IPC_DEMO_MAGIC || last_reply.kind != IPC_DEMO_KIND_REPLY) {
		return;
	}

	k_sem_give(&reply_sem);
}

static void ep_error(const char *message, void *priv)
{
	LOG_ERR("IPC error: %s", message);
}

static struct ipc_ept_cfg ep_cfg = {
	.cb =
		{
			.bound = ep_bound,
			.received = ep_recv,
			.error = ep_error,
		},
};

int ipc_demo_host_run(void)
{
	const struct device *ipc = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	struct ipc_ept ep;
	int ret;

	if (!device_is_ready(ipc)) {
		LOG_ERR("ipc0 device not ready");
		return -ENODEV;
	}

	ret = ipc_service_open_instance(ipc);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("ipc_service_open_instance failed: %d", ret);
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc, &ep, &ep_cfg);
	if (ret < 0) {
		LOG_ERR("ipc_service_register_endpoint failed: %d", ret);
		return ret;
	}

	ret = k_sem_take(&ep_bound_sem, K_SECONDS(5));
	if (ret != 0) {
		LOG_ERR("endpoint bind timeout");
		(void)ipc_service_deregister_endpoint(&ep);
		return ret;
	}

	struct ipc_demo_msg hello = {
		.magic = IPC_DEMO_MAGIC,
		.kind = IPC_DEMO_KIND_HELLO,
		.seq = 1,
	};
	strncpy(hello.payload, "hello", sizeof(hello.payload) - 1);

	for (;;) {
		ret = ipc_service_send(&ep, &hello, sizeof(hello));
		if (ret == (int)sizeof(hello)) {
			break;
		}
		if (ret == -ENOMEM) {
			k_yield();
			continue;
		}
		LOG_ERR("ipc_service_send failed: %d", ret);
		(void)ipc_service_deregister_endpoint(&ep);
		return ret;
	}

	ret = k_sem_take(&reply_sem, K_SECONDS(3));
	if (ret != 0) {
		LOG_ERR("reply timeout");
		(void)ipc_service_deregister_endpoint(&ep);
		return -ETIMEDOUT;
	}

	LOG_INF("IPC reply: seq=%u text=\"%s\"", last_reply.seq, last_reply.payload);

	ret = ipc_service_deregister_endpoint(&ep);
	if (ret < 0) {
		LOG_WRN("ipc_service_deregister_endpoint: %d", ret);
	}

	return 0;
}
