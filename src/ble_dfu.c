/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_dfu, LOG_LEVEL_INF);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void advertising_start(void)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to restart (err %d)", err);
	} else {
		LOG_INF("Advertising as \"%s\"", CONFIG_BT_DEVICE_NAME);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}
	LOG_INF("Connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason %u)", reason);
}

static void recycled(void)
{
	/* Connection object released after disconnnect - restart advertiser */
	advertising_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = recycled,
};

static int ble_init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("bt_enable failed (err %d)", err);
		return err;
	}
	LOG_INF("Bluetooth initialised");

	advertising_start();

	return 0;
}

/* Run after kernel init, before application main. */
SYS_INIT(ble_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
