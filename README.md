# CanoKey on ESP32 (WIP)

CanoKey is an open-source USB/NFC security token, providing the following functions:

- OpenPGP Card V3.4 (RSA, ECDSA, ED25519 supported)
- PIV Card
- TOTP / HOTP (RFC6238 / RFC4226)
- U2F
- FIDO2 (WebAuthn)

It works on modern Linux/Windows/macOS operating systems without additional driver required.

**THERE IS ABSOLUTELY NO SECURITY ASSURANCE OR WARRANTY USING THE ESP32 VERSION.**

**ANYONE WITH PHYSICAL ACCESS CAN RETRIEVE ANY DATA (INCLUDING SECRET KEYS) FROM THE KEY.**

**IT IS ALSO PRONE TO SIDE-CHANNEL ATTACKS.**

**YOU MUST USE IT AT YOUR OWN RISK.**

**A SECURE VERSION CAN BE FOUND AT https://canokeys.org**

## Status

This is a WIP. Code is not cleaned up and some functions are not implemented currectly (dummy stub).

Functions like `device_delay`, atomic functions, LED, and touch detection are not implemented.

The USB stack is now a mixed(缝合) stack, namely driver from tinyUSB but the whole stack from STM32. `usbd_conf.c` is totally a mess. `dcd_esp32sx.c` also needs less hack. The whole stack should be migrated to tinyUSB later.

Currently many crypto operations may time out or not work due to lack of implementation. I have tested that generating RSA2048 key did not work. Also, signing using RSA4096 would timeout. However, signing using RSA2048/Ed25519 worked.

## Hardware

This is currently based on ESP32-S2, with ESP32-S2-DevKitC-1 as the development board.

However, this should also work on ESP32-S3 as their USB driver is the same. 

## Build

0. Clone the repo and all the submodules

1. Setup the ESP-IDF environment, assume it is in `$IDF_PATH`,
   ```bash
   source $IDF_PATH/export.sh
   ```

2. Then configure the project. In `Partition Table` section you should use `Custom Partition Table CSV` (Make sure you have at least 2MB flash) and in `Component config -> ESP System Settings` change `Task Watchdog timeout period` to 60.
   ```bash
   idf.py set-target esp32s2
   idf.py menuconfig
   ```

3. Then flash and play
   ```bash
   sudo -E idf.py -p /dev/ttyUSB0 flash monitor
   ```

## Initlialize and Test

Ref to <https://github.com/canokeys/canokey-stm32#initialize-and-test>. You should change the PIN in the script from `313131313131` to `313233343536`
