/*
 * bootloaderMain.h
 *
 *  Created on: 16 Apr 2023
 *      Author: Javier
 */

#ifndef INC_BOOTLOADERMAIN_H_
#define INC_BOOTLOADERMAIN_H_

#include "main.h"

#define mainAPPstartFlashAddr 0x8010000//flash memory available for the bootloader

typedef enum {
  YES=1,
  NO=0
} BootloaderLogicEnum ;

BootloaderLogicEnum isthereSDcard();
BootloaderLogicEnum isThereFirmwarefile();
BootloaderLogicEnum doFlashandSDfirmwarecontentsMatch();
void jumpToApp();
BootloaderLogicEnum eraseFLASHappSpace();
BootloaderLogicEnum programfromRAMtoFLASH();

#endif /* INC_BOOTLOADERMAIN_H_ */
