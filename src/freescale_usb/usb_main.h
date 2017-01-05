/*
 * File:		usb_main.h
 * Purpose:		Main process
 *
 */

#include <stdint.h>

void usb_main_init (void);
int usb_main_mainfunction(uint8_t *pChar);
void usb_send_data(uint8_t *pu8DataPointer,uint8_t u8DataSize);
