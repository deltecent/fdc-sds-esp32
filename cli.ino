void cliSetup(Stream* defaultConsole) {
  cliConsole = defaultConsole;

  strcpy(cliPrompt, "ESP32 FDC+>");

  cli.setOnError(errorCallback);  // Set error Callback

  cmdHelp = cli.addCommand("h/elp,?", helpCallback);
  cmdBaud = cli.addBoundlessCommand("b/aud,speed", baudCallback);
  cmdDir = cli.addBoundlessCommand("d/ir,ls", dirCallback);
  cmdMount = cli.addBoundlessCommand("m/ount", mountCallback);
  cmdUnmount = cli.addBoundlessCommand("u/nmount", unmountCallback);
  cmdStats = cli.addCommand("s/tat/s", statsCallback);
  cmdSave = cli.addCommand("save,w/rite", saveCallback);
  cmdErase = cli.addCommand("erase", eraseCallback);
  cmdDump = cli.addCommand("d/u/mp", dumpCallback);
  cmdWifi = cli.addBoundlessCommand("wifi", wifiCallback);
  cmdSSID = cli.addBoundlessCommand("ssid", ssidCallback);
  cmdPass = cli.addBoundlessCommand("pass", passCallback);
  cmdReboot = cli.addCommand("reboot", rebootCallback);
  cmdUpdate = cli.addCommand("update", updateCallback);
  cmdVersion = cli.addCommand("v/er/sion", versionCallback);
  cmdType = cli.addBoundlessCommand("t/ype,cat", typeCallback);
  cmdExec = cli.addBoundlessCommand("e/xec,run", execCallback);
  cmdLogout = cli.addCommand("logout,exit", logoutCallback);
  cmdDelete = cli.addBoundlessCommand("del/ete,rm", deleteCallback);
  cmdRename = cli.addBoundlessCommand("ren/ame,mv", renameCallback);
  cmdClear = cli.addCommand("clear", clearCallback);
  
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
        if (cliConsole == &TelnetStream) {
          TelnetStream.disconnectClient();
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
          cliBuf[cliIdx--] = 0;
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
  cliConsole->printf("DELETE filename           Delete file\r\n");
  cliConsole->printf("DIR                       Directory\r\n");
  cliConsole->printf("DUMP                      Dump track buffer\r\n");
  cliConsole->printf("ERASE                     Erase configuration\r\n");
  cliConsole->printf("EXEC filename             Execute filename\r\n");
  if (cliConsole == &TelnetStream) {
    cliConsole->printf("LOGOUT                    Logout\r\n");
  }
  cliConsole->printf("MOUNT [drive filename]    Mount drive\r\n");
  cliConsole->printf("PASS pass                 Set WiFi password\r\n");
  cliConsole->printf("REBOOT                    Reboot device\r\n");
  cliConsole->printf("RENAME old new            Rename file\r\n");
  cliConsole->printf("SAVE                      Save configuration\r\n");
  cliConsole->printf("SSID ssid                 Set WiFi SSID\r\n");
  cliConsole->printf("STATS                     Statistics\r\n");
  cliConsole->printf("TYPE filename             Display file\r\n");
  cliConsole->printf("UNMOUNT drive             Unmount drive\r\n");
  cliConsole->printf("UPDATE                    Update firmware\r\n");
  cliConsole->printf("VERSION                   Dispay version\r\n");
  cliConsole->printf("WIFI ON | OFF             Turn WiFi On and Off\r\n");
}

void versionCallback(cmd* c) {
  cliConsole->printf("%d.%d\r\n", MAJORVER, MINORVER);
}

void logoutCallback(cmd* c) {
  if (cliConsole == &TelnetStream) {
    TelnetStream.disconnectClient();
    cliIdx = 0;
    cliBuf[0] = 0;
  }
}

void rebootCallback(cmd* c) {
  cliConsole->printf("Rebooting...\r\n");
  SD.end();
  TelnetStream.disconnectClient();
  delay(1000);
  WiFi.disconnect();
  delay(1000);
  ESP.restart();
}

void updateCallback(cmd* c) {
  cliConsole->printf("Not implemented\r\n");
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

  while (f.available()) {
    cli.parse(f.readStringUntil('\n'));
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
        cliConsole->printf("%-25.25s (%d Tracks)\r\n", drive[d].filename+1, drive[d].tracks);
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

  for (int d = 0; d < MAX_DRIVE; d++) {
    sprintf(key, "Drive%d", d);
    fdcPrefs.putString(key, drive[d].filename);
  }

  cliConsole->printf("Configuration saved.\r\n");

  confChanged = false;
}

// Callback function for erase command
void eraseCallback(cmd* c) {
  nvs_flash_erase();  // erase the NVS partition and...
  nvs_flash_init();   // initialize the NVS partition.

  cliConsole->printf("Configuration erased.\r\n");

  confChanged = false;

  loadPrefs();
}

// Callback function for dump command
void dumpCallback(cmd* c) {
  cliConsole->printf("D:%02d T:%04d Track Buffer:\r\n", lastDrive, lastTrack);
  dumpBuffer(trackBuf, sizeof(trackBuf));
}

// Callback function for stats command
void statsCallback(cmd* c) {
  Command cmd(c);  // Create wrapper object

  int argNum = cmd.countArgs();  // Get number of arguments

  cliConsole->printf("FDC+ Baud Rate: %d\r\n\n", baudRate);

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
    wifiDisconnect();
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