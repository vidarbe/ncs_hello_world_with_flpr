/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HELLO_WORLD_WITH_FLPR_IPC_DEMO_H_
#define HELLO_WORLD_WITH_FLPR_IPC_DEMO_H_

/**
 * Run a minimal ipc_service handshake over DT_NODELABEL(ipc0) (icmsg backend).
 * Blocks until a reply is received from the FLPR image or a timeout occurs.
 *
 * @return 0 on success, negative errno otherwise
 */
int ipc_demo_host_run(void);

/** FLPR image entry: blocks handling one hello/reply exchange then returns. */
void ipc_demo_remote_main(void);

#endif /* HELLO_WORLD_WITH_FLPR_IPC_DEMO_H_ */
