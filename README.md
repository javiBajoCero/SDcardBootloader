# SDcardBootloader
HAL based CUBEMX generated project, finds a *.bin file inside the SD card and runs it in flash.    
Based in the Adafruit's feather stm32f405 board  https://github.com/javiBajoCero/Adafruit-Feather-STM32F405-Express-PCB  

## System features are tested here:    
https://github.com/javiBajoCero/featherSDcardTests




## APP.bin files stored inside firmware_folder to be flashed in chips memory ,special considerations
The Firmware "APP" that gets loaded and executed by this bootloader is going to be stored in FLASH memory at address 0x8010000, its vector table needs to be offseted 0x0010000.
Changes need to be made in `system_stm32f4xx.c`
![image](https://user-images.githubusercontent.com/25673527/232601442-1b48b112-e407-4a26-9c20-8e958c538a95.png)

## How the bootloader works (broad view)
![image](https://user-images.githubusercontent.com/25673527/232600822-11da3f84-657a-4a74-abf6-07e3e30ebd21.png)
