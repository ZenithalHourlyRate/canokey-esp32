/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_conf.c
  * @version        : v2.0_Cube
  * @brief          : This file implements the board support package for the USB device library
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "dcd_esp32sx.h"
#include "usbd_def.h"
#include "usbd_core.h"

/* USER CODE BEGIN Includes */
#include "usb_device.h"
//#include "device-config.h"
#include "usbd_ccid.h"
#include "usbd_ctaphid.h"
#include "usbd_kbdhid.h"

#include "esp_rom_sys.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

static int ctrl_status = 0;
static int set_address = 0;
static int is_transmit = 0;
static uint8_t transmit_ep = 0;
static const uint8_t *transmit_buffer = NULL;
static uint16_t transmit_size = 0;

void Error_Handler(void);

/* Exported function prototypes ----------------------------------------------*/
extern USBD_StatusTypeDef USBD_LL_BatteryCharging(USBD_HandleTypeDef *pdev);

/*******************************************************************************
                       LL Driver Callbacks (PCD -> USB Device Library)
*******************************************************************************/
/* MSP Init */

/**
  * @brief  Setup stage callback
  * @param  hpcd: PCD handle
  * @retval None
  */
void dcd_event_setup_received(uint8_t* setup_packet)
{
  esp_rom_printf("Setup");
  for (int i = 0; i != 8; ++i)
    esp_rom_printf(" %02X", setup_packet[i]);
  esp_rom_printf("\n");
  USBD_LL_SetupStage(&usb_device, setup_packet);
}

/**
  * @brief  Data Out stage callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint number
  * @retval None
  */
void dcd_event_xfer_complete(uint8_t ep_addr, uint8_t* buffer)
{
  esp_rom_printf("Xfer complete\n");
  if (is_transmit && ep_addr == transmit_ep) {
    // manual LL_Transmit to edpt_xfer fragementation
    uint16_t new_xfer_size = dcd_edpt_xfer(transmit_ep, transmit_buffer, transmit_size);
    if (new_xfer_size < transmit_size) {
      transmit_buffer += new_xfer_size;
      transmit_size -= new_xfer_size; 
    } else {
      is_transmit = 0;
    }
  } else if (ep_addr & 0x80) { // IN
    esp_rom_printf("  IN %02X\n", ep_addr);
    if (set_address) {
      set_address = 0;
      return; 
    }
    if (ep_addr == 0x80) {
      dcd_edpt_xfer(0, NULL, 0);
      ctrl_status = 1;
    } else
    {
      USBD_LL_DataInStage(&usb_device, ep_addr & 0x7F, NULL);
    }
  } else {
    esp_rom_printf("  OUT %02X\n", ep_addr);
    if (ctrl_status) {
      // ignore status xfer out
      ctrl_status = 0;
      return;
    }
    USBD_LL_DataOutStage(&usb_device, ep_addr, buffer);
    //// First IN wont be delivered to stack (no complete callback)
    //USBD_LL_DataInStage(&usb_device, ep_addr, buffer);
  }
}

/**
  * @brief  SOF callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void dcd_event_bus_sof()
{
  esp_rom_printf("SOF\n");
  USBD_LL_SOF(&usb_device);
}

/**
  * @brief  Reset callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void dcd_event_bus_reset()
{ 
  esp_rom_printf("Reset\n");

  /* Set Speed. */
  USBD_SpeedTypeDef speed = USBD_SPEED_FULL;
  USBD_LL_SetSpeed(&usb_device, speed);

  /* Reset Device. */
  USBD_LL_Reset(&usb_device);
}

/**
  * @brief  Suspend callback.
  * When Low power mode is enabled the debug cannot be used (IAR, Keil doesn't support it)
  * @param  hpcd: PCD handle
  * @retval None
  */
void dcd_event_bus_suspend()
{
  /* Inform USB library that core enters in suspend Mode. */
  USBD_LL_Suspend(&usb_device);
  /* Enter in STOP mode. */
}

/**
  * @brief  Resume callback.
  * When Low power mode is enabled the debug cannot be used (IAR, Keil doesn't support it)
  * @param  hpcd: PCD handle
  * @retval None
  */
void dcd_event_bus_resume()
{
  USBD_LL_Resume(&usb_device);
}

///**
//  * @brief  Connect callback.
//  * @param  hpcd: PCD handle
//  * @retval None
//  */
//#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
//static void PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
//#else
//void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
//#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
//{
//  USBD_LL_DevConnected(&usb_device);
//}

/**
  * @brief  Disconnect callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void dcd_event_bus_disconnected()
{
  esp_rom_printf("Disconnect\n");
  USBD_LL_DevDisconnected(&usb_device);
}

/*******************************************************************************
                       LL Driver Interface (USB Device Library --> PCD)
*******************************************************************************/

/**
  * @brief  Initializes the low level portion of the device driver.
  * @param  pdev: Device handle
  * @retval USBD status
  */
USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
  esp_rom_printf("Init\n");
  dcd_init();
  return USBD_OK;
}

/**
  * @brief  De-Initializes the low level portion of the device driver.
  * @param  pdev: Device handle
  * @retval USBD status
  */
USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
  (void)pdev;
  esp_rom_printf("DeInit\n");
  return USBD_OK;
}

/**
  * @brief  Starts the low level portion of the device driver.
  * @param  pdev: Device handle
  * @retval USBD status
  */
USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
  (void)pdev;
  esp_rom_printf("Start\n");
  dcd_connect();
  return USBD_OK;
}

/**
  * @brief  Stops the low level portion of the device driver.
  * @param  pdev: Device handle
  * @retval USBD status
  */
USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
  (void)pdev;
  esp_rom_printf("Stop\n");
  dcd_disconnect();
  return USBD_OK;
}

/**
  * @brief  Opens an endpoint of the low level driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint number
  * @param  ep_type: Endpoint type
  * @param  ep_mps: Endpoint max packet size
  * @retval USBD status
  */
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t ep_type, uint16_t ep_mps)
{
  (void)pdev;
  esp_rom_printf("OpenEP: ep %d type %d mps %d\n", ep_addr, ep_type, ep_mps);
  dcd_edpt_open(ep_addr, ep_type, ep_mps);
  return USBD_OK;
}

/**
  * @brief  Closes an endpoint of the low level driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint number
  * @retval USBD status
  */
USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  // TODO
  (void)pdev;
  return USBD_OK;
}

/**
  * @brief  Flushes an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint number
  * @retval USBD status
  */
USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  // TODO
  (void)pdev;
  return USBD_OK;
}

/**
  * @brief  Sets a Stall condition on an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint number
  * @retval USBD status
  */
USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  (void)pdev;
  esp_rom_printf("StallEP: ep %d\n", ep_addr);
  dcd_edpt_stall(ep_addr);
  return USBD_OK;
}

/**
  * @brief  Clears a Stall condition on an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint number
  * @retval USBD status
  */
USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  (void)pdev;
  esp_rom_printf("ClearStallEP, ep %d\n", ep_addr);
  dcd_edpt_clear_stall(ep_addr);
  return USBD_OK;
}

/**
  * @brief  Returns Stall condition.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint number
  * @retval Stall (1: Yes, 0: No)
  */
uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  return 0;
}

/**
  * @brief  Assigns a USB address to the device.
  * @param  pdev: Device handle
  * @param  dev_addr: Device address
  * @retval USBD status
  */
USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr)
{
  (void)pdev;
  esp_rom_printf("SetUSBAddress addr %d\n", dev_addr);
  dcd_set_address(dev_addr);
  set_address = 1; // dcd_set_address would send a IN ZLP packet
  return USBD_OK;
}

/**
  * @brief  Transmits data over an endpoint.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint number
  * @param  pbuf: Pointer to data to be sent
  * @param  size: Data size    
  * @retval USBD status
  */
USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t ep_addr, const uint8_t *pbuf, uint16_t size)
{
  (void)pdev;
  if (ep_addr == 0x00) ep_addr = 0x80;
  esp_rom_printf("Transmit: ep %d len %d", ep_addr, size);
  for (int i = 0; i != size; ++i)
    esp_rom_printf(" %02X", pbuf[i]);
  esp_rom_printf("\n");
  uint16_t xfer_size = dcd_edpt_xfer(ep_addr, pbuf, size);
  if (xfer_size < size) {
    is_transmit = 1;
    transmit_ep = ep_addr;
    transmit_buffer = pbuf + xfer_size;
    transmit_size = size - xfer_size; 
  }
  return USBD_OK;
}

/**
  * @brief  Prepares an endpoint for reception.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint number
  * @param  pbuf: Pointer to data to be received
  * @param  size: Data size
  * @retval USBD status
  */
USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint16_t size)
{
  (void)pdev;
  esp_rom_printf("Prepare: ep %d len %d\n", ep_addr, size);
  dcd_edpt_prepare(ep_addr, pbuf, size);
  return USBD_OK;
}

/**
  * @brief  Returns the last transfered packet size.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint number
  * @retval Recived Data Size
  */
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  (void)pdev;
  return dcd_edpt_get_rx_size(ep_addr);
}

/**
  * @brief  Delays routine for the USB Device Library.
  * @param  Delay: Delay in ms
  * @retval None
  */
void USBD_LL_Delay(uint32_t Delay)
{
  // TODO
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
