#include <Preferences.h>
#include <FS.h>
#include <SD.h>
#include <WiFi.h>
#include <SimpleCLI.h>
#include <HttpsOTAUpdate.h>
#include <nvs_flash.h>

#define MAJORVER  0
#define MINORVER  2

HardwareSerial fdcSerial(2);

Preferences fdcPrefs;

int baudRate = 230400;

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

#define DEFAULT_BAUD  230400

// Statistics
uint32_t statCnt = 0;
uint32_t readCnt = 0;
uint32_t writCnt = 0;
uint32_t errsCnt = 0;
uint32_t toutCnt = 0;

#define MAX_DRIVE  4

typedef struct DRIVE {
  char  mounted;
  char  filename[30];
  int16_t tracks;
} drive_t;

drive_t drive[MAX_DRIVE];

#define MAX_TRACK 2048
#define TRACKSIZE (137 * 32)

int dSelLED[MAX_DRIVE] = {13, 12, 14, 27};

uint8_t trackBuf[TRACKSIZE];
int lastTrack = -1;
int lastDrive = -1;
char lastStat[40];
char lastRead[40];
char lastWrit[40];
char lastErr[80];

// Create CLI Object
SimpleCLI cli;
char cliPrompt[10];
char cliBuf[80] = {0};
int cliIdx = 0;

// Commands
Command cmdHelp;
Command cmdBaud;
Command cmdDir;
Command cmdMount;
Command cmdUnmount;
Command cmdStats;
Command cmdSave;
Command cmdErase;
Command cmdDump;
Command cmdWifi;
Command cmdSSID;
Command cmdPass;
Command cmdReboot;
Command cmdUpdate;
Command cmdVersion;
Command cmdType;
Command cmdExec;

bool confChanged = false;
bool sdReady = false;

void flushrx(void) {
  while (fdcSerial.available()) {
    fdcSerial.read();
    delay(10);
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

  char key[10];

  for (int d=0; d<MAX_DRIVE; d++) {
    drive[d].filename[0] = 0;
    drive[d].mounted = false;

    sprintf(key, "Drive%d", d);

    if (fdcPrefs.isKey(key)) {
      fdcPrefs.getString(key, drive[d].filename, sizeof(drive[d].filename));

      if (drive[d].filename[0]) {
        mountDrive(d, drive[d].filename);
      }
    }
  }

  confChanged = false;
}

void sdSetup() {
  if (!SD.begin(5)) {
    Serial.println("Could not initialize SD card");
    return;
  }

  sdReady = true;

  uint16_t sectorSize = SD.sectorSize();
  uint16_t bytesRead;
  uint32_t sector = 0;
  uint8_t *buffer = NULL;

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
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

void setup() {
  // put your setup code here, to run once:
  //pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);

  Serial.printf("\r\nESP32 FDC+ Serial Drive Server %d.%d\r\n", MAJORVER, MINORVER);

  // Built-in LED for Head Load
  pinMode(LED_BUILTIN, OUTPUT);

  sdSetup();

  diskSetup();

  loadPrefs();

  wifiSetup();

  fdcSetup();

  cliSetup();
}

void loop() {
  // put your main code here, to run repeatedly:

  //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

  cliInput();
  fdcProc();
}
