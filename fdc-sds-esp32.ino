/*----------------------------------------------------------------------------------
;
;  FDC+ Serial Disk Server for ESP32 DEVKIT 1
;      This program serves disk images over a high speed serial port
;      using the FDC+ Serial Drive Server protocol.
;
;     Version  Date        Author         Notes
;      1.0     01/05/2024  P. Linstruth   Original
;
;-------------------------------------------------------------------------------------*/

#include <Preferences.h>
#include <FS.h>
#include <SD.h>
#include <WiFi.h>
#include <Update.h>
#include <ESPTelnetStream.h>
#include <SimpleCLI.h>
#include "SimpleFTPServer.h"
#include <nvs_flash.h>

#if !defined ( ESP32 )
	#error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

#define MAJORVER  0
#define MINORVER  15

HardwareSerial fdcSerial(2);
ESPTelnetStream telnet;

Preferences fdcPrefs;

#define DEFAULT_BAUD  403200;

int baudRate = DEFAULT_BAUD;

bool wifiEnabled = false;
char wifiSSID[80];
char wifiPass[80];

/*
** FDC+ Command / Response Block
*/
typedef struct CRBLOCK {
  union {
    byte block[8];

    struct {
      byte cmd[4];
      byte word1[2];
      byte word2[2];
      //byte checksum[2];
    };
  };
} crblk_t;

#define LSB 0
#define MSB 1

// Statistics
uint32_t statCnt = 0;
uint32_t readCnt = 0;
uint32_t writCnt = 0;
uint32_t errsCnt = 0;
volatile uint32_t toutCnt = 0;

#define MAX_DRIVE  4

typedef struct DRIVE {
  char mounted;
  char filename[30];
  int size;
  File diskImg;
} drive_t;

drive_t drive[MAX_DRIVE];

int dSelLED[MAX_DRIVE] = {27, 14, 12, 13};

uint8_t trackBuf[1024 * 8];
int lastTrack = -1;
int lastDrive = -1;
int lastLen = -1;
char lastStat[80] = {0};
char lastRead[80] = {0};
char lastWrit[80] = {0};
char lastErr[80] = {0};

// Create CLI Object
SimpleCLI cli;
char cliPrompt[10] = {0};
char cliBuf[80] = {0};
int cliIdx = 0;
Stream *cliConsole = &Serial;

// Commands
Command cmdHelp;
Command cmdBaud;
Command cmdDir;
Command cmdMount;
Command cmdUnmount;
Command cmdStats;
Command cmdSave;
Command cmdWipe;
Command cmdDump;
Command cmdWifi;
Command cmdSSID;
Command cmdPass;
Command cmdReboot;
Command cmdUpdate;
Command cmdVersion;
Command cmdType;
Command cmdExec;
Command cmdLogout;
Command cmdDelete;
Command cmdRename;
Command cmdClear;
Command cmdCopy;
Command cmdLoopback;

bool confChanged = false;
bool sdReady = false;

FtpServer ftpSrv;

// FDC+ timeout timer
#define FDC_TIMEOUT_MS 75

hw_timer_t *fdcTimer = NULL;
volatile bool fdcTimeout = false;

void timerSetup() {
  // Set timer frequency to 1Mhz
  fdcTimer = timerBegin(1000000);

   // Attach onTimer function to our timer.
  timerAttachInterrupt(fdcTimer, &fdcTimerISR);

  // Set alarm to call fdcTimerISR function every second (value in microseconds).
  // Repeat the alarm (third parameter) with unlimited count = 0 (fourth parameter).
  timerAlarm(fdcTimer, FDC_TIMEOUT_MS * 1000, true, 0);
}

void flushrx(Stream *s) {
  while (s->available()) {
    s->read();
//    delay(100);
  }
}

void loadPrefs() {
  fdcPrefs.begin("fdc", false);

  if (fdcPrefs.isKey("baudRate")) {
    baudRate = fdcPrefs.getInt("baudRate");
    Serial.printf("FDC+ baud rate: %d\r\n", baudRate);
  } else {
    baudRate = DEFAULT_BAUD;
    Serial.printf("Setting FDC+ default baud rate to %d\r\n", baudRate);
  }

  if (fdcPrefs.isKey("wifiEnabled")) {
    wifiEnabled = fdcPrefs.getBool("wifiEnabled");
  }
  Serial.printf("WiFi: %sEnabled\r\n", (wifiEnabled) ? "" : "Not ");

  wifiSSID[0] = 0;
  if (fdcPrefs.isKey("wifiSSID")) {
    fdcPrefs.getString("wifiSSID", wifiSSID, sizeof(wifiSSID));
  }

  wifiPass[0] = 0;
  if (fdcPrefs.isKey("wifiPass")) {
    fdcPrefs.getString("wifiPass", wifiPass, sizeof(wifiPass));
  }



  confChanged = false;
}

bool sdSetup() {
  if (!SD.begin(5,SPI,4000000,"/sd",8)) {
    Serial.print("Could not initialize SD card\r\n");
    return false;
  }

  sdReady = true;

  uint16_t sectorSize = SD.sectorSize();

  Serial.printf("Sector Size: %d\r\n", SD.sectorSize());

  uint8_t cardType = SD.cardType();

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\r\n", cardSize);

  // Mount disks
  diskSetup();

  return true;
}

void setup() {
  Serial.begin(115200);

  delay(500);

  flushrx(&Serial);

  Serial.printf("\r\nESP32 FDC+ Serial Drive Server %d.%d\r\n\r\n", MAJORVER, MINORVER);

  // FDC status LED
  pinMode(LED_BUILTIN, OUTPUT);

  sdSetup();

  loadPrefs();

  wifiSetup();

  diskSetup();
  
  timerSetup();

  fdcSetup();

  cliSetup(&Serial);
}

void loop() {
  cliInput(&Serial, true);

  if (WiFi.status() == WL_CONNECTED) {
    telnet.loop();
    if (telnet.isConnected()) {
      cliInput(&telnet, false);
    }

    ftpSrv.handleFTP();
  }

  fdcProc();
}
