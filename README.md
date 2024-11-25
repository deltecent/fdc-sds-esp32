> [!CAUTION]
> This site is currently under construction... use at your own risk!

## FDC+ Enhanced Floppy Disk Controller for the Altair 8800

The Altair FDC+ is an enhanced version of the original MITS 8" floppy disk controller for the Altair 8800. The FDC+ is a 100% compatible drop-in replacement for the original two-board Altair FDC. The FDC+ can serve as a replacement for a missing or defective Altair FDC, or you can use the FDC+ as a reference point while restoring original floppy equipment to working condition.

In addition to providing disk controller functions, the FDC+ includes up to 64K of RAM and 8K of PROM that can be separately enabled if needed. This RAM and PROM can come in very handy when trying to get an Altair up and running. The RAM can be enabled in 1K increments. The PROM can be enabled in 256 byte increments.

The FDC+ includes a built-in, high speed serial port that can connect to a PC running a serial disk server. From the perspective of the Altair computer and software, it appears that an Altair/Pertec drive is attached. Performance of the serial drive is virtually identical to the original Altair floppy drive. Both the Altair 8" drive and Minidisk drive are supported. Numerous disk images – available via links on the FDC+ web page – can be mounted on the PC server and run from your Altair computer. Disk images include a customized version of CP/M that provides 8Mb capacity on drives A & B and standard Altair drives on C & D.

[Watch the FDC+ in Action](https://youtu.be/1U013-9eB1A)

(https://www.circuitstate.com/pinouts/doit-esp32-devkit-v1-wifi-development-board-pinout-diagram-and-reference/)

## ESP32 FDC+ Serial Disk Server

The ESP32 FDC+ Serial Disk Server ("ESP32-SDS") provides an alternative to the Windows-based server.

## Building the FDC+ Serial Disk Server with Separate Components

While a custom ESP32-SDS is being a developed, an ESP32-SDS prototype can be built using parts from Amazon.

[ESP-WROOM-32 Development Board](https://www.amazon.com/dp/B07WCG1PLV)  
[ESP32 Breakout Board](https://www.amazon.com/dp/B0BNQ85GF3)  
[Adafruit MicroSD Card Breakout Board+](https://www.amazon.com/dp/B00NAY2NAI)  
[2PCS MAX3232 3.3V to 5V DB9 Male RS232 Serial Prot to TTL Converter Module](https://www.amazon.com/dp/B07LBDZ9WG) (Note 1)
[Ableconn PI232DB9M Compact GPIO TX/RX to DB9M RS232 Serial Expansion Board](https://www.amazon.com/dp/B00WPBXDJC)
[SanDisk 32GB Ultra microSDHC UHS-I Memory Card](https://www.amazon.com/dp/B073JWXGNT)  
[2PCS Micro SD Card Module TF Card Memory Storage Adapter Reader Board](https://www.amazon.com/dp/B08C4WY2WR)  
[RGB LED Module for ESP32](https://www.amazon.com/dp/B0BXKMGSG6)  

Note 1. This serial expansion board has a maximum baud rate of 230.4K.

## ESP32 DEV KIT 1 Pinout

![ESP32 DEV KIT 1 PINOUT](https://mischianti.org/wp-content/uploads/2020/11/ESP32-DOIT-DEV-KIT-v1-pinout-mischianti.png)

```
FDC+ RS232 (VCC or 3.3V as required)

RX2 - RXD
TX2 - TXD

SD Card (VCC or 3.3V as required)

D5  - CS
D18 - CLK
D19 - MISO
D23 - MOSI

Head Load LEDs

D13 - Drive 0
D12 - Drive 1
D14 - Drive 2
D27 - Drive 3
```

## Compiling the Firmware

Install the [Arduino IDE](https://www.arduino.cc/en/software).

In the Board Manager, install "esp32 by Espressif Systems" and select the "DOIT ESP32 DEVKIT 1" board.

Install the following libraries:

* ESP Telnet
* SimpleCLI

Compile and upload the Firmware.

