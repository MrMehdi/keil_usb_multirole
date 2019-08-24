/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::USB:Device
 * Copyright (c) 2004-2014 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    HID.c
 * Purpose: USB Device Human Interface Device example program
 *----------------------------------------------------------------------------*/

#include "cmsis_os.h"
#include "rl_usb.h"

int main (void) {
  uint8_t buf[1];

  USBD_Initialize    (0);               /* USB Device 0 Initialization        */
  USBD_Connect       (0);               /* USB Device 0 Connect               */

  while (1) {                           /* Loop forever                       */
    // USBD_HID_GetReportTrigger(0, 0, &buf[0], 1);
    osDelay(100);                       /* 100 ms delay for sampling buttons  */
  }
}
