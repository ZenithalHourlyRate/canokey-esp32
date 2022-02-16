/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Scott Shawcroft, 2019 William D. Jones for Adafruit Industries
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 * Additions Copyright (c) 2020, Espressif Systems (Shanghai) Co. Ltd.
 * Copyright (c) 2022, Hongren Zheng (Canokeys.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 * Modified for canokey-esp32
 */

#include <stdint.h>
#include <stdbool.h>

/*------------------------------------------------------------------*/
/* Controller API
 *------------------------------------------------------------------*/

void dcd_init();
void dcd_set_address(uint8_t dev_addr);
void dcd_remote_wakeup();
void dcd_connect();
void dcd_disconnect();

/*------------------------------------------------------------------*/
/* DCD Endpoint port
 *------------------------------------------------------------------*/

bool dcd_edpt_open(uint8_t ep_addr, uint8_t ep_type, uint16_t ep_mps);
void dcd_edpt_close_all();
uint16_t dcd_edpt_xfer(uint8_t ep_addr, const uint8_t *buffer, uint16_t total_bytes);
uint32_t dcd_edpt_get_rx_size(uint8_t ep_addr);
void dcd_edpt_prepare(uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes);
void dcd_edpt_stall(uint8_t ep_addr);
void dcd_edpt_clear_stall(uint8_t ep_addr);

/*------------------------------------------------------------------*/
/* DCD Interrupt
 *------------------------------------------------------------------*/

void dcd_int_enable ();
void dcd_int_disable ();

/*------------------------------------------------------------------*/
/* DCD Callback
 *------------------------------------------------------------------*/

extern void dcd_event_setup_received(uint8_t* setup_packet);
extern void dcd_event_xfer_complete(uint8_t ep_addr, uint8_t* buffer);
extern void dcd_event_bus_reset();
extern void dcd_event_bus_suspend();
extern void dcd_event_bus_resume();
extern void dcd_event_bus_disconnected();
extern void dcd_event_bus_sof();
