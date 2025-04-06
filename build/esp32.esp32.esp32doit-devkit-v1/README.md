This folder contains a binary images of the ESP32-FDC-SDS firmware.

Download the file `fdc-sds-esp32.ino.bin`, store it on your microSD card as `update.bin`, and then run the `update` command.

```
ESP32 FDC+>copy fdc-sds-esp32.ino.bin update.bin
Copied 1046624 bytes
ESP32 FDC+>update
Starting update
Wrote : 1046624 successfully
Update successful.
Rebooting...
ets Jul 29 2019 12:21:46

rst:0xc (SW_CPU_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:1
load:0x3fff0030,len:4604
ho 0 tail 12 room 4
load:0x40078000,len:15488
load:0x40080400,len:4
load:0x40080404,len:3180
entry 0x400805b8

ESP32 FDC+ Serial Drive Server 0.21

Sector Size: 512
SD Card Type: SDHC
SD Card Size: 29721MB
FDC+ baud rate: 460800
WiFi: Not Enabled
ESP32 FDC+>
```

