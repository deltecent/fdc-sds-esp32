void cliSetup(Stream* defaultConsole) {
  cliConsole = defaultConsole;

  strcpy(cliPrompt, "ESP32 FDC+>");

  cli.setOnError(errorCallback);  // Set error Callback

  cmdHelp = cli.addCommand("h/elp,?", helpCallback);
  cmdBaud = cli.addBoundlessCommand("b/aud,speed", baudCallback);
  cmdDir = cli.addBoundlessCommand("d/ir,ls", dirCallback);
  cmdMount = cli.addBoundlessCommand("m/ount", mountCallback);
  cmdUnmount = cli.addBoundlessCommand("u/nmount,umount", unmountCallback);
  cmdStats = cli.addCommand("s/tat/s", statsCallback);
  cmdSave = cli.addCommand("save,w/rite", saveCallback);
  cmdWipe = cli.addCommand("wipe", wipeCallback);
  cmdDump = cli.addCommand("d/u/mp", dumpCallback);
  cmdWifi = cli.addBoundlessCommand("wifi", wifiCallback);
  cmdSSID = cli.addBoundlessCommand("ssid", ssidCallback);
  cmdPass = cli.addBoundlessCommand("pass", passCallback);
  cmdName = cli.addBoundlessCommand("host/name", hostnameCallback);
  cmdReboot = cli.addCommand("reboot", rebootCallback);
  cmdUpdate = cli.addCommand("update", updateCallback);
  cmdVersion = cli.addCommand("v/er/sion", versionCallback);
  cmdType = cli.addBoundlessCommand("t/ype,cat", typeCallback);
  cmdExec = cli.addBoundlessCommand("e/xec,run", execCallback);
  cmdLogout = cli.addCommand("logout,exit", logoutCallback);
  cmdDelete = cli.addBoundlessCommand("del/ete,era,rm", deleteCallback);
  cmdRename = cli.addBoundlessCommand("ren/ame,mv", renameCallback);
  cmdClear = cli.addCommand("clear", clearCallback);
  cmdCopy = cli.addBoundlessCommand("copy,cp",copyCallback);
  cmdLoopback = cli.addCommand("loopback,lb", loopbackCallback);
  cmdTime = cli.addCommand("time,date", timeCallback);
  
  dispPrompt();
}

void dispPrompt() {
  if (confChanged) {
    cliConsole->print("* ");
  }
  cliConsole->print(cliPrompt);
}

// Get input
void cliInput(Stream* console, bool echo) {
  byte c;
  static byte lastChar;

  cliConsole = console;

  while (cliConsole->available() > 0) {
    c = cliConsole->read();

    switch (c) {
      case 0x00:
        break;

      case 0x04:
        if (cliConsole == &telnet) {
          telnet.disconnectClient();
          cliIdx = 0;
          cliBuf[0] = 0;
        }
        break;

      case '\r':
      case '\n':
        if (c == '\n' && lastChar == '\r') {
          break;
        }

        if (echo) {
          cliConsole->printf("\r\n");
        }

        cli.parse(cliBuf);
        dispPrompt();

        cliIdx = 0;
        cliBuf[0] = 0;
        break;

      case '\b':
      case 127:
        if (cliIdx) {
          if (echo) {
            cliConsole->print("\b \b");
          }
          cliBuf[--cliIdx] = 0;
        }
        break;

      default:
        if (cliIdx >= sizeof(cliBuf) - 2) {
          cliConsole->print("\a");
          return;
        }

        if (echo) {
          cliConsole->printf("%c", c);
        }
        cliBuf[cliIdx++] = c;
        cliBuf[cliIdx] = 0;
        break;
    }

    lastChar = c;
  }
}

void helpCallback(cmd* c) {
  cliConsole->printf("BAUD baud                 Set FDC+ baud rate\r\n");
  cliConsole->printf("CLEAR                     Clear statistics\r\n");
  cliConsole->printf("COPY src dst              Copy file from src to dst\r\n");
  cliConsole->printf("DELETE filename           Delete file\r\n");
  cliConsole->printf("DIR                       Directory\r\n");
  cliConsole->printf("DUMP                      Dump track buffer\r\n");
  cliConsole->printf("EXEC filename             Execute filename\r\n");
  cliConsole->printf("HOSTNAME name             Set Wifi hostname\r\n");
  if (cliConsole == &telnet) {
    cliConsole->printf("LOGOUT                    Logout\r\n");
  }
  cliConsole->printf("LOOPBACK                  FDC+ loopback test\r\n");
  cliConsole->printf("MOUNT [drive filename]    Mount drive\r\n");
  cliConsole->printf("PASS pass                 Set WiFi password\r\n");
  cliConsole->printf("REBOOT                    Reboot device\r\n");
  cliConsole->printf("RENAME old new            Rename file\r\n");
  cliConsole->printf("SAVE                      Save configuration to NVRAM\r\n");
  cliConsole->printf("SSID ssid                 Set WiFi SSID\r\n");
  cliConsole->printf("STATS                     FDC+ Statistics\r\n");
  cliConsole->printf("TIME                      Display time\r\n");
  cliConsole->printf("TYPE filename             Display file\r\n");
  cliConsole->printf("UNMOUNT drive             Unmount drive\r\n");
  cliConsole->printf("UPDATE                    Update firmware\r\n");
  cliConsole->printf("VERSION                   Dispay version\r\n");
  cliConsole->printf("WIPE                      Wipe NVRAM configuration\r\n");
  cliConsole->printf("WIFI [ON | OFF]           Turn WiFi On and Off\r\n");
}

void versionCallback(cmd* c) {
  cliConsole->printf("%d.%d\r\n", MAJORVER, MINORVER);
}

void logoutCallback(cmd* c) {
  if (cliConsole == &telnet) {
    telnet.disconnectClient();
    cliIdx = 0;
    cliBuf[0] = 0;
  }
}

void rebootCallback(cmd* c) {
  cliConsole->printf("Rebooting...\r\n");
  SD.end();
  wifiEnabled = false;
  WiFi.disconnect();
  delay(1000);
  ESP.restart();
}

void updateCallback(cmd* c) {
  File updateBin = SD.open("/update.bin");
  int r;

  if (updateBin) {
    if (updateBin.isDirectory()) {
      cliConsole->println("Error, update.bin is not a file");
      updateBin.close();
      return;
    }

    size_t updateSize = updateBin.size();

    if (updateSize > 0) {
      cliConsole->println("Starting update");
      r = performUpdate(updateBin, updateSize);
    } else {
      cliConsole->println("Error, file is empty");
    }

    updateBin.close();

    // when finished remove the binary from sd card to indicate end of the process
    if (r) {
      SD.remove("/update.bin");
      rebootCallback(c);  
    }
  } else {
    cliConsole->println("Could not load update.bin from SD root");
  }
}

// perform the actual update from a given stream
bool performUpdate(Stream &updateSource, size_t updateSize) {
  if (Update.begin(updateSize)) {
    size_t written = Update.writeStream(updateSource);
    if (written == updateSize) {
      cliConsole->println("Wrote : " + String(written) + " successfully");
    } else {
      cliConsole->println("Wrote only : " + String(written) + "/" + String(updateSize) + ". Retry?");
      return false;
    }
    if (Update.end()) {
      if (Update.isFinished()) {
        cliConsole->println("Update successful.");
        return true;
      } else {
        cliConsole->println("Update not finished? Something went wrong!");
      }
    } else {
      cliConsole->println("Error Occurred. Error #: " + String(Update.getError()));
    }

  } else {
    cliConsole->println("Not enough space to begin OTA");
  }

  return false;
}

void timeCallback(cmd* c) {
  struct tm timeinfo;

  time_t now = time(nullptr);
  gmtime_r(&now, &timeinfo);

  cliConsole->printf("%.24s\r\n", F(asctime(&timeinfo)));
}

void loopbackCallback(cmd* c) {
  byte sendBuf[256];
  byte recvBuf[256];
  int rcvd;

  for (int i=0; i<sizeof(sendBuf); i++) {
    sendBuf[i] = i;
  }

  cliConsole->printf("FDC+ Loopback Test\r\n\n");
  cliConsole->printf("Press any key to quit...\r\n");

  while (!cliConsole->available()) {
    cliConsole->print("+");
    sendBlock(sendBuf, sizeof(sendBuf), false, 0);
    if (!recvBlock(recvBuf, sizeof(recvBuf), 1000)) {
      cliConsole->printf("%s\r\n", lastErr);
      for (int i=0; i<sizeof(sendBuf); i++) {
        if (sendBuf[i] != recvBuf[i]) {
          cliConsole->printf("%04X: %02X != %02X\r\n", i, sendBuf[i], recvBuf[i]);
        }
      }
      delay(5000);
    } else {
      cliConsole->print("-");
    }
  }

  cliConsole->read();
}

void copyCallback(cmd* c) {
  Command cmd(c);
  int bytes = 0;
  int copied = 0;
  uint8_t buf[4096];

  int argNum = cmd.countArgs();  // Get number of arguments

  if (argNum != 2) {
    cliConsole->printf("copy src dst\r\n");
    return;
  }

  Argument srcArg = cmd.getArg(0);
  String srcFilename = "/" + srcArg.getValue();
  Argument dstArg = cmd.getArg(1);
  String dstFilename = "/" + dstArg.getValue();

  File src = SD.open(srcFilename, "r");

  if (!src) {
    cliConsole->printf("Could not open source file '%s'\r\n", srcFilename.c_str());
    return;
  }

  File dst = SD.open(dstFilename, "w");

  if (!dst) {
    cliConsole->printf("Could not open destination file '%s'\r\n", dstFilename.c_str());
    src.close();
    return;
  }

  while (src.available()) {
    bytes = src.read(buf, sizeof(buf));
    dst.write(buf, bytes);
 
    copied += bytes;

    fdcProc();
   }

  src.close();
  dst.close();

  cliConsole->printf("Copied %d bytes\r\n", copied);
}

void typeCallback(cmd* c) {
  Command cmd(c);
  char ch;

  int argNum = cmd.countArgs();  // Get number of arguments

  if (!argNum) {
    cliConsole->printf("type filename\r\n");
    return;
  }

  Argument filenameArg = cmd.getArg(0);
  String filename = "/" + filenameArg.getValue();

  File f = SD.open(filename);

  if (!f) {
    cliConsole->printf("Could not open file\r\n");
    return;
  }

  while (f.available()) {
    String s = f.readStringUntil('\n');
    cliConsole->print(s + "\r\n");
  }

  f.close();
}

void execCallback(cmd* c) {
  Command cmd(c);
  char ch;

  int argNum = cmd.countArgs();  // Get number of arguments

  if (!argNum) {
    cliConsole->printf("type filename\r\n");
    return;
  }

  Argument filenameArg = cmd.getArg(0);
  String filename = "/" + filenameArg.getValue();

  File f = SD.open(filename);

  if (!f) {
    f = SD.open(filename + ".bat");
    if (!f) {
      cliConsole->printf("Could not open file\r\n");
      return;
    }
  }

  String s;

  while (f.available()) {
    s = f.readStringUntil('\n');
    s.trim();
    if (s.charAt(0) == '#') {
      cliConsole->printf("%s\r\n", s.c_str());
    } else {
      cli.parse(s);
    }
  }

  f.close();
}

void baudCallback(cmd* c) {
  Command cmd(c);

  int argNum = cmd.countArgs();  // Get number of arguments

  if (argNum != 1) {
    cliConsole->printf("baud [9600, 19200, 38400, 57600, 76800, 230400, 403200, 460800 ]\r\n");
    return;
  }

  Argument driveArg = cmd.getArg(0);
  String driveValue = driveArg.getValue();

  if (baudRate != driveValue.toInt()) {
    baudRate = driveValue.toInt();
    confChanged = true;
    fdcBaudrate();
  }
}

// Callback function for dir command
void dirCallback(cmd* c) {
  Command cmd(c);  // Create wrapper object

  int argNum = cmd.countArgs();  // Get number of arguments

  listDir(SD, "/", 0);
}

// Callback function for delete command
void deleteCallback(cmd* c) {
  Command cmd(c);  // Create wrapper object

  int argNum = cmd.countArgs();  // Get number of arguments

  // If no arguments, dump mount table
  if (argNum != 1) {
    cliConsole->printf("delete <filename>\r\n");
    return;
  }

  Argument fileArg = cmd.getArg(0);
  String fileValue = "/" + fileArg.getValue();

  if (!SD.remove(fileValue)) {
    cliConsole->printf("Could not remove file\r\n");
  }
}


// Callback function for rename command
void renameCallback(cmd* c) {
  Command cmd(c);  // Create wrapper object

  int argNum = cmd.countArgs();  // Get number of arguments

  // If no arguments, dump mount table
  if (argNum != 2) {
    cliConsole->printf("rename <old> <new>\r\n");
    return;
  }

  Argument oldArg = cmd.getArg(0);
  String oldValue = "/" + oldArg.getValue();

  Argument newArg = cmd.getArg(1);
  String newValue = "/" + newArg.getValue();

  SD.rename(oldValue, newValue);
}

// Callback function for mount command
void mountCallback(cmd* c) {
  Command cmd(c);  // Create wrapper object

  int argNum = cmd.countArgs();  // Get number of arguments

  // If no arguments, dump mount table
  if (!argNum) {
    for (int d = 0; d < MAX_DRIVE; d++) {
      cliConsole->printf("Drive %d: ", d);
      if (drive[d].mounted) {
        cliConsole->printf("%-30.30s %d\r\n", drive[d].filename+1, drive[d].size);
      } else {
        cliConsole->printf("[ NOT MOUNTED ]\r\n");
      }
    }
    return;
  } else if (argNum != 2) {
    cliConsole->printf("mount [drive] [filename]\r\n");
    return;
  }

  Argument driveArg = cmd.getArg(0);
  String driveValue = driveArg.getValue();
  int d = driveValue.toInt();

  if (d < 0 || d > MAX_DRIVE) {
    cliConsole->printf("Drive must be between 0 and %d\r\n", MAX_DRIVE);
    return;
  }
  Argument filenameArg = cmd.getArg(1);
  String filename = "/" + filenameArg.getValue();
  mountDrive(d, filename.c_str());
}

// Callback function for unmount command
void unmountCallback(cmd* c) {
  Command cmd(c);  // Create wrapper object

  int argNum = cmd.countArgs();  // Get number of arguments

  // If no arguments, dump mount table
  if (!argNum) {
    cliConsole->printf("unmount [drive]\r\n");
    return;
  }

  Argument driveArg = cmd.getArg(0);
  String driveValue = driveArg.getValue();
  int d = driveValue.toInt();

  if (d < 0 || d > MAX_DRIVE) {
    cliConsole->printf("Drive must be between 0 and %d\r\n", MAX_DRIVE - 1);
    return;
  }

  unmountDrive(d);
}

// Callback function for save command
void saveCallback(cmd* c) {
  char key[10];

  fdcPrefs.putInt("baudRate", baudRate);
  fdcPrefs.putBool("wifiEnabled", wifiEnabled);
  fdcPrefs.putString("wifiSSID", wifiSSID);
  fdcPrefs.putString("wifiPass", wifiPass);
  fdcPrefs.putString("wifiName", wifiName);

  for (int d = 0; d < MAX_DRIVE; d++) {
    sprintf(key, "Drive%d", d);
    fdcPrefs.putString(key, drive[d].filename);
  }

  cliConsole->printf("Configuration saved.\r\n");

  confChanged = false;
}

// Callback function for wipe command
void wipeCallback(cmd* c) {
  nvs_flash_erase();  // erase the NVS partition and...
  nvs_flash_init();   // initialize the NVS partition.

  cliConsole->printf("Configuration erased.\r\n");

  confChanged = false;

  loadPrefs();
}

// Callback function for dump command
void dumpCallback(cmd* c) {
  cliConsole->printf("D:%02d T:%04d Track Buffer:\r\n", lastDrive, lastTrack);
  dumpBuffer(trackBuf, lastLen);
}

// Callback function for stats command
void statsCallback(cmd* c) {
  Command cmd(c);  // Create wrapper object

  int argNum = cmd.countArgs();  // Get number of arguments

  cliConsole->printf("FDC+ Connection Status: %s\r\n\n", (fdcTimeout) ? "Not Connected" : "Connected");

  cliConsole->printf("FDC+ Baud Rate: %d (%d)\r\n\n", baudRate, fdcSerial.baudRate());

  cliConsole->printf("STAT: %08d\r\n", statCnt);
  cliConsole->printf("READ: %08d\r\n", readCnt);
  cliConsole->printf("WRIT: %08d\r\n", writCnt);
  cliConsole->printf("ERRS: %08d\r\n", errsCnt);
  cliConsole->printf("TOUT: %08d\r\n", toutCnt);

  cliConsole->printf("Last STAT : %s\r\n", lastStat);
  cliConsole->printf("Last READ : %s\r\n", lastRead);
  cliConsole->printf("Last WRIT : %s\r\n", lastWrit);
  cliConsole->printf("Last Error: %s\r\n", lastErr);
}

// Callback function for clear command
void clearCallback(cmd* c) {
  statCnt = 0;
  readCnt = 0;
  writCnt = 0;
  errsCnt = 0;
  toutCnt = 0;

  lastStat[0] = 0;
  lastRead[0] = 0;
  lastWrit[0] = 0;
  lastErr[0] = 0;

  lastDrive = -1;
  lastTrack = -1;
  
  statsCallback(c);
}

// Callback function for stats command
void wifiCallback(cmd* c) {
  Command cmd(c);  // Create wrapper object
  bool prevWifi = wifiEnabled;

  int argNum = cmd.countArgs();  // Get number of arguments

  if (!argNum) {
    cliConsole->printf("WiFi %s\r\n\n", (wifiEnabled) ? "Enabled" : "Disabled");
    cliConsole->printf("WiFi SSID: %s\r\n", (strlen(wifiSSID)) ? wifiSSID : "Not Set");
    cliConsole->printf("WiFi PASS: %s\r\n\n", (strlen(wifiPass)) ? "******" : "Not Set");
    cliConsole->printf("WiFi Status: %s\r\n", (WiFi.status() == WL_CONNECTED) ? "Connected" : "Not Connected");
    cliConsole->printf("WiFi IP Address: %s\r\n", WiFi.localIP().toString().c_str());
    cliConsole->printf("WiFi Hostname: %s\r\n", wifiName);
    return;
  }

  Argument cmdArg = cmd.getArg(0);
  String onOff = cmdArg.getValue();
  onOff.toUpperCase();

  if (onOff.startsWith("ON")) {
    wifiEnabled = true;
  } else if (onOff.startsWith("OF")) {
    wifiEnabled = false;
  } else {
    cliConsole->printf("Invalid option\r\n");
    return;
  }

  if (wifiEnabled && !prevWifi) {
    wifiSetup();
    confChanged = true;
  } else if (!wifiEnabled && prevWifi) {
    wifiDisconnected();
    confChanged = true;
  }
}

// Callback function for ssid command
void ssidCallback(cmd* c) {
  Command cmd(c);  // Create wrapper object

  int argNum = cmd.countArgs();  // Get number of arguments

  if (!argNum) {
    cliConsole->printf("ssid SSID\r\n");
    return;
  }

  Argument cmdArg = cmd.getArg(0);
  String ssid = cmdArg.getValue();

  if (strcmp(wifiSSID, ssid.c_str())) {
    strncpy(wifiSSID, ssid.c_str(), sizeof(wifiSSID));
    confChanged = true;

    wifiDisconnected();  // WiFi should auto reconnect if enabled
  }
}

// Callback function for hostname command
void hostnameCallback(cmd* c) {
  Command cmd(c);  // Create wrapper object

  int argNum = cmd.countArgs();  // Get number of arguments

  if (!argNum) {
    cliConsole->printf("hostname name\r\n");
    return;
  }

  Argument cmdArg = cmd.getArg(0);
  String hostname = cmdArg.getValue();

  if (strcmp(wifiName, hostname.c_str())) {
    strncpy(wifiName, hostname.c_str(), sizeof(wifiName));
    confChanged = true;

    wifiSetup();
  }
}

// Callback function for pass command
void passCallback(cmd* c) {
  Command cmd(c);  // Create wrapper object

  int argNum = cmd.countArgs();  // Get number of arguments

  if (!argNum) {
    cliConsole->printf("pass PASSWORD\r\n");
    return;
  }

  Argument cmdArg = cmd.getArg(0);
  String pass = cmdArg.getValue();

  if (strcmp(wifiPass, pass.c_str())) {
    strncpy(wifiPass, pass.c_str(), sizeof(wifiPass));
    confChanged = true;

    wifiDisconnected();
    wifiSetup();
  }
}

// Callback in case of an error
void errorCallback(cmd_error* e) {
  CommandError cmdError(e);  // Create wrapper object
  Command cmd;

  String data = cmdError.getData();
  data.toUpperCase();

  // If a batch file exists, run it!
  if (data.endsWith(".BAT")) {
    cli.parse("exec " + data);
    return;
  } else if (SD.exists("/" + data + ".bat")) {
    cli.parse("exec " + data + ".bat");
    return;
  }

  cliConsole->print("ERROR: ");
  cliConsole->println(cmdError.toString());

  cliConsole->println(cmdError.getCommand().getName());

  if (cmdError.hasCommand()) {
    cliConsole->print("Did you mean \"");
    cliConsole->print(cmdError.getCommand().toString());
    cliConsole->println("\"?");
  }
}