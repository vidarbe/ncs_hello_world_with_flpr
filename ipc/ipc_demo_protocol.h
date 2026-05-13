/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared layout for the cpuapp ↔ FLPR IPC demo (see Zephyr
 * samples/subsys/ipc/ipc_service/icmsg for the reference flow).
 */

#ifndef HELLO_WORLD_WITH_FLPR_IPC_DEMO_PROTOCOL_H_
#define HELLO_WORLD_WITH_FLPR_IPC_DEMO_PROTOCOL_H_

#include <stdint.h>

#define IPC_DEMO_MAGIC 0x64706369U /* "ipcd" */

enum ipc_demo_kind {
	IPC_DEMO_KIND_HELLO = 1,
	IPC_DEMO_KIND_REPLY = 2,
};

struct ipc_demo_msg {
	uint32_t magic;
	uint32_t kind;
	uint32_t seq;
	char payload[24];
};

#endif /* HELLO_WORLD_WITH_FLPR_IPC_DEMO_PROTOCOL_H_ */
