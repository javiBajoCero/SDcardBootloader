/*
 * bootloaderMain.c
 *
 *  Created on: 16 Apr 2023
 *      Author: Javier
 */

#include "bootloaderMain.h"
#include "fatfs.h"
#include "string.h"
#include "usart.h"
#include "adc.h"
#include "sdio.h"

#define APPsize (64*0x400)

    FATFS fs;
    FIL myFILE;
    UINT testByte;
    FRESULT stat;
    DIR directory;
    FILINFO filinfo;

    /////////////////////////////////////////////////////blind jump to memory struct
    typedef void (application_t)(void);
    typedef struct
    {
        uint32_t		stack_addr;     // Stack Pointer
        application_t*	func_p;        // Program Counter
    } JumpStruct;
    //////////////////////////////////////////////////////
    const char directorypath[]="firmware_folder";
    char fullfilepath[100]={0};

    char buffer[APPsize]={0};//64KB of ram

BootloaderLogicEnum isSDcardMounted();
BootloaderLogicEnum isthereafirmwarefolder(const TCHAR* path);
BootloaderLogicEnum isthereafirmwarefile();
BootloaderLogicEnum compareRAMbufferwithFlashcontents(char * rambuffer, char * flashbuffer, UINT size);

void deinitEverything();


BootloaderLogicEnum isthereSDcard(){
	if(HAL_GPIO_ReadPin(SD_DETECT_GPIO_Port, SD_DETECT_Pin)==GPIO_PIN_SET){//SD card detected!
		return YES;
	}else{//SD card not detected
		return NO;
	}
}


BootloaderLogicEnum isThereFirmwarefile(){
	//try to mount SD card
	if(isSDcardMounted()==NO){
		return NO;
	}
	//is there a firmware folder?
	if(isthereafirmwarefolder(directorypath)==NO){
		return NO;
	}
	//is there a firmware.bin in that folder?
	if(isthereafirmwarefile(directorypath)==NO){
		return NO;
	}
	//is the firmware.bin size bigger than -....

	return YES;
}

BootloaderLogicEnum isSDcardMounted(){

	// Re-initialize SD
	if ( BSP_SD_Init() != MSD_OK ) {
	  return NO;
	}

	// Re-initialize FATFS
	if ( FATFS_UnLinkDriver(SDPath) != 0 ) {
	  return NO;
	}
	if ( FATFS_LinkDriver(&SD_Driver, SDPath) != 0 ) {
	  return NO;
	}

	// Mount filesystem
	stat = f_mount(&fs, SDPath, 1);
	if (stat != FR_OK) {
		return NO;
	}


	return YES;
}

BootloaderLogicEnum isthereafirmwarefolder(const TCHAR* path){
	if(f_opendir(&directory, path)!=FR_OK){
		return NO;
	}
	return YES;
}

BootloaderLogicEnum isthereafirmwarefile(){
	if(f_findfirst(&directory, &filinfo, directorypath, "*.bin")!=FR_OK
			&& (filinfo.fsize>0)
			&& (filinfo.fsize<=APPsize)){
		return NO;
	}
	return YES;

}

BootloaderLogicEnum doFlashandSDfirmwarecontentsMatch(){
	UINT bytesreadfromfile=0;
	fullfilepath[0]=0;
	strcat(fullfilepath,directorypath);
	strcat(fullfilepath,"/");
	strcat(fullfilepath,filinfo.fname);

	if(f_open(&myFILE,fullfilepath, FA_READ)!=FR_OK){
		return NO;
	}

	if((f_read(&myFILE, buffer, filinfo.fsize, &bytesreadfromfile)!=FR_OK) && (bytesreadfromfile== filinfo.fsize)){
		return NO;
	}

	if(compareRAMbufferwithFlashcontents(buffer, (char *)mainAPPstartFlashAddr, bytesreadfromfile)!=YES){
		return NO;
	}

    // Sync, close file, unmount
    stat = f_close(&myFILE);
    f_mount(0, SDPath, 0);

	return YES;

}

BootloaderLogicEnum compareRAMbufferwithFlashcontents(char * rambuffer, char * flashbuffer, UINT size){
	for (uint32_t i = 0; i < size; ++i) {
		if(rambuffer[i]!=flashbuffer[i]){
			return NO;
		}
	}
	return YES;
}

BootloaderLogicEnum eraseFLASHappSpace(){
	FLASH_EraseInitTypeDef erase;
	HAL_StatusTypeDef status;
	uint32_t SectorError=0;
	erase.TypeErase=FLASH_TYPEERASE_SECTORS;
	erase.Sector=FLASH_SECTOR_4;//0x8010000 //RM0090 pag75
	erase.NbSectors=2;//4-5
	erase.VoltageRange=FLASH_VOLTAGE_RANGE_3;//https://electronics.stackexchange.com/questions/200992/can%C2%B4t-erase-data-from-flash-memory-stm32
	status=HAL_FLASH_Unlock();
		if(status!=HAL_OK){return NO;}
	status=HAL_FLASHEx_Erase(&erase, &SectorError);
		if(status!=HAL_OK){return NO;}
	status=HAL_FLASH_Lock();
		if(status!=HAL_OK){return NO;}

	return YES;
}

BootloaderLogicEnum programfromRAMtoFLASH(){
	HAL_FLASH_Unlock();
	for (uint32_t i = 0; i < filinfo.fsize; ++i) {//https://community.st.com/s/question/0D50X00009XkXWXSA3/stm32f446re-flashtypeprogramdoubleword-doesnt-work
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, mainAPPstartFlashAddr+i, buffer[i]);
	}
	HAL_FLASH_Lock();
	return YES;
}

void deinitEverything() {
	//-- reset peripherals to guarantee flawless start of user application
	//	HAL_GPIO_DeInit(LED_GPIO_Port, LED_Pin);
	HAL_UART_DeInit(&huart3);
	HAL_SD_DeInit(&hsd);
	HAL_ADC_DeInit(&hadc1);
	HAL_RCC_DeInit();
	HAL_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;
}

void jumpToApp(){
	const JumpStruct *vector_p = (JumpStruct*)mainAPPstartFlashAddr;
	deinitEverything();
	/* let's do The Jump! */
	/* Jump, used asm to avoid stack optimization */
	asm("msr msp, %0; bx %1;" : : "r"(vector_p->stack_addr), "r"(vector_p->func_p));
}

