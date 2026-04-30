# Hello World with FLPR + BLE DFU

This sample demonstrates running code on both the application core
and the FLPR (Fast Lightweight Peripheral Processor) on the nRF54L15 SoC.
It also include DFU over BLE support for updating the FW of both.

## Overview

The application core boots first, brings up Bluetooth and the SMP
(MCUmgr) DFU transport, then loads and starts the FLPR firmware.
The FLPR image is built as a separate sysbuild image and its
`zephyr.bin` is converted into a C array that is embedded inside the
application image.

### Notes

- **Single-image DFU.** The FLPR firmware is embedded inside the
  application binary as a C array, so MCUboot only sees one updatable
  image. A single OTA update will update both the application and
  coprocessor firmware atomically.
- **FLPR booting.** `CONFIG_NORDIC_VPR_LAUNCHER` is disabled so
  `main()` controls when the FLPR starts, after BLE / DFU has come up.
- **BLE DFU** Uses MCUboot + MCUmgr SMP over BLE,
  compatible with `nRF Connect Device Manager` (mobile) and the nRF Util 
  `mcu-manager` command. 
  (https://docs.nordicsemi.com/bundle/nrfutil/page/nrfutil-mcu-manager/nrfutil-mcu-manager.html)

## Project layout

```
hello_world_with_flpr/
├── CMakeLists.txt                  Application core CMake; embeds FLPR blob
├── Kconfig.sysbuild                Resolves FLPR board target from app target
├── VERSION                         Application version (used by app_version.h)
├── prj.conf                        App core Kconfig (BT, MCUmgr, MCUboot)
├── sysbuild.cmake                  Adds FLPR as a sysbuild sub-image
├── sysbuild.conf                   Enables MCUboot at the sysbuild level
├── memory_map.dtsi                 Shared MCUboot/app RRAM partition layout
├── boards/
│   └── nrf54l15dk_nrf54l15_cpuapp.overlay   App core SRAM split + FLPR memory
├── coprocessor/                    FLPR image
│   ├── CMakeLists.txt
│   ├── prj.conf
│   ├── boards/
│   │   └── nrf54l15dk_nrf54l15_cpuflpr.overlay
│   └── src/main.c
├── scripts/
│   └── bin_to_header.py            Converts zephyr.bin to C array
├── src/
│   ├── main.c                      App core: starts/stops the FLPR
│   └── ble_dfu.c                   Brings up BLE + SMP transport via SYS_INIT
└── sysbuild/
    └── mcuboot/                    MCUboot Kconfig + DT overlay
        ├── prj.conf
        └── boards/
            └── nrf54l15dk_nrf54l15_cpuapp.overlay
```

## Build flow

Sysbuild builds three images: MCUboot, the application, and the FLPR coprocessor. Since
the coprocessor is linked into RAM, the zephyr.bin/zephyr.hex contains only .text, .rodata and .data 
destined for RAM, with no flash sections.

```
            ┌────────────────────┐
            │ coprocessor (FLPR) │  ─────┐
            └────────────────────┘       │  zephyr.bin
                                         ▼
                            scripts/bin_to_header.py
                                         │
                                         ▼ flpr_firmware.h
            ┌────────────────────┐       │
            │ application (app)  │ ◄─────┘
            └────────────────────┘
```

## Memory map (RRAM)

| Partition       | Offset      | Size   | Notes                    |
| --------------- | ----------- | ------ | ------------------------ |
| `mcuboot`       | `0x000000`  |  32 KB | Bootloader               |
| `slot0`         | `0x008000`  | 736 KB | Active application       |
| `slot1`         | `0x0C0000`  | 736 KB | DFU staging slot         |
| `storage`       | `0x178000`  |  20 KB | Settings storage         |

Total application area: **1524 KB**. The FLPR binary is part of
slot0/slot1 and is not stored separately in flash.

## Building

Prerequisites:

- nRF Connect SDK **v3.3.0** 
- A nRF54L15 DK

Build with sysbuild:

```bash
west build -b nrf54l15dk/nrf54l15/cpuapp --sysbuild .
```

Flash all images (application + MCUboot):

```bash
west flash --erase 
```
Note: Using the `--erase` option will also provision the bootloader signature key to the 
KMU during flashing. This is required by the bootloader to validate and boot the application. If you are using the VS Code extension, hover over the Flash button to reveal a new button on the right. Clicking this button is equivalent to running west flash --erase.

After flashing, connect to the VCOM ports exposed by the DK to view the log messages 
from the FLPR and application core.

```
*** Booting nRF Connect SDK ...
*** Using Zephyr OS ... ***
Hello world from nrf54l15dk/nrf54l15/cpuflpr
```

```
*** Booting My Application ... ***
*** Using nRF Connect SDK ... ***
*** Using Zephyr OS ... ***
[00:00:00.017,869] <inf> bt_sdc_hci_driver: SoftDevice Controller build revision: ...       
[00:00:00.019,102] <inf> bt_hci_core: HW Platform: Nordic Semiconductor (0x0002)
[00:00:00.019,115] <inf> bt_hci_core: HW Variant: nRF54Lx (0x0005)
[00:00:00.019,126] <inf> bt_hci_core: Firmware: Standard Bluetooth controller (0x00)
[00:00:00.019,518] <inf> bt_hci_core: HCI transport: SDC
[00:00:00.019,566] <inf> bt_hci_core: Identity: xx:xx:xx:xx:xx:xx (random)
[00:00:00.019,582] <inf> bt_hci_core: HCI: version 6.2 (0x10) revision 0x30a3, manufacturer 0x0059
[00:00:00.019,603] <inf> bt_hci_core: LMP: version 6.2 (0x10) subver 0x30a3
[00:00:00.019,608] <inf> ble_dfu: Bluetooth initialised
[00:00:00.020,091] <inf> ble_dfu: Advertising as "DFU Service"
[00:00:00.020,144] <inf> app: Hello world from nrf54l15dk/nrf54l15/cpuapp. Version: ...
[00:00:00.020,150] <inf> app: Copying N bytes of FLPR firmware to SRAM @ 0x20028000 ...
```

## Performing DFU

Once running, the device advertises as **"DFU Service"** .

### Using `nRF Connect` (Android / iOS)

1. Edit `VERSION` and bump the version, then rebuild.
2. Locate `build/hello_world_with_flpr/dfu_application.zip`.
3. Copy zip file to phone.
3. In the app, scan for **DFU Service**, connect, open
   DFU tab, and select the dfu_application.zip and start DFU.
4. Check the debug log to confirm that the app version number has been
   updated on the device.

## bin_to_header.py

A small utility that converts any binary file into a C header
exposing it as a `static const uint8_t` array plus a size macro:

```bash
python3 scripts/bin_to_header.py \
    --input  build/coprocessor/zephyr/zephyr.bin \
    --output build/include/flpr_firmware.h \
    --array-name flpr_firmware
```

Generates:

```c
#define FLPR_FIRMWARE_SIZE  N
static const uint8_t flpr_firmware[] = { 0x..., 0x..., ... };
```
## About this project

This application is one of several applications that has been built by the support team at Nordic Semiconductor, as a demo of some particular feature or use case. It has not necessarily been thoroughly tested, so there might be unknown issues. It is hence provided as-is, without any warranty.