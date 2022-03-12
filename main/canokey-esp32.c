#include <stdio.h>

// espressif
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_private/periph_ctrl.h"
#include "esp_private/usb_phy.h"
#include "soc/usb_pins.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// canokey-core
#include "common.h"
#include "apdu.h"
#include "applets.h"
#include "device.h"
#include "usb_device.h"

#include "dcd_esp32sx.h"
#include "littlefs_api.h"

static usb_phy_handle_t phy_hdl;
static const char* TAG = "canokey-esp32";

esp_err_t usb_driver_install()
{
  // Configure USB PHY
  usb_phy_config_t phy_conf = {
      .controller = USB_PHY_CTRL_OTG,
      .otg_mode = USB_OTG_MODE_DEVICE,
      .target = USB_PHY_TARGET_INT,
  };
  ESP_RETURN_ON_ERROR(usb_new_phy(&phy_conf, &phy_hdl), TAG, "Install USB PHY failed");

  dcd_init();
  dcd_int_enable();
  ESP_LOGI(TAG, "TinyUSB Driver installed");
  return ESP_OK;
}

#ifndef HW_VARIANT_NAME
#define HW_VARIANT_NAME "CanoKey ESP32"
#endif

int admin_vendor_hw_variant(const CAPDU *capdu, RAPDU *rapdu) {
  UNUSED(capdu);

  static const char *const hw_variant_str = HW_VARIANT_NAME;
  size_t len = strlen(hw_variant_str);
  memcpy(RDATA, hw_variant_str, len);
  LL = len;
  if (LL > LE) LL = LE;

  return 0;
}

int admin_vendor_hw_sn(const CAPDU *capdu, RAPDU *rapdu) {
  UNUSED(capdu);

  static const char *const hw_sn = "20220313";
  memcpy(RDATA, hw_sn, 8);
  LL = 8;
  if (LL > LE) LL = LE;

  return 0;
}

int strong_user_presence_test(void) {
  DBG_MSG("Strong user-presence test is skipped.\n");
  return 0; 
}

void device_delay(int tick) {
  return;
}
uint32_t device_get_tick(void) {
  return 0;
}
void device_disable_irq(void) {}
void device_enable_irq(void) {}
void device_set_timeout(void (*callback)(void), uint16_t timeout) {}
void fm_write_eeprom(uint16_t addr, uint8_t *buf, uint8_t len) { return; }

int device_atomic_compare_and_swap(volatile uint32_t *var, uint32_t expect, uint32_t update) {
  if (*var == expect) {
    *var = update;
    return 0;
  } else {
    return -1;
  }
}

int device_spinlock_lock(volatile uint32_t *lock, uint32_t blocking) {
  // Not really working, for test only
  while (*lock) {
    if (!blocking) return -1;
  }
  *lock = 1;
  return 0;
}
void device_spinlock_unlock(volatile uint32_t *lock) { *lock = 0; }
void led_on(void) {}
void led_off(void) {}

/* Override the function defined in usb_device.c */
void usb_resources_alloc(void) {
  uint8_t iface = 0;
  uint8_t ep = 1;

  memset(&IFACE_TABLE, 0xFF, sizeof(IFACE_TABLE));
  memset(&EP_TABLE, 0xFF, sizeof(EP_TABLE));

  EP_TABLE.ctap_hid = ep++;
  IFACE_TABLE.ctap_hid = iface++;
  EP_SIZE_TABLE.ctap_hid = 64;

  IFACE_TABLE.webusb = iface++;

  EP_TABLE.ccid = ep++;
  IFACE_TABLE.ccid = iface++;
  EP_SIZE_TABLE.ccid = 64;

  //if (cfg_is_kbd_interface_enable() && ep <= EP_ADDR_MSK) {
  //  DBG_MSG("Keyboard interface enabled, Iface %u\n", iface);
  //  EP_TABLE.kbd_hid = ep;
  //  IFACE_TABLE.kbd_hid = iface;
  //  EP_SIZE_TABLE.kbd_hid = 8;
  //}
}

void app_main(void)
{
  littlefs_init();

  // avoid touch test
  set_nfc_state(1);

  usb_device_init();
  usb_driver_install();

  // Step 6: init applets
  DBG_MSG("Init applets\n");
  applets_install();
  init_apdu_buffer();
  DBG_MSG("Done\n");

  while(1) {
    device_loop(0);
    vTaskDelay(10);
  }
}
