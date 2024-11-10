void cliSetup() {
  strcpy(cliPrompt, "ESP32 FDC+>");

  cli.setOnError(errorCallback); // Set error Callback

  cmdHelp = cli.addCommand("h/elp,?", helpCallback);
  cmdBaud = cli.addBoundlessCommand("b/aud,speed", baudCallback);
  cmdDir = cli.addCommand("d/ir,ls", dirCallback);
  cmdMount = cli.addBoundlessCommand("m/ount", mountCallback);
  cmdUnmount = cli.addBoundlessCommand("u/nmount", unmountCallback);
  cmdStats = cli.addCommand("s/tat/s", statsCallback);
  cmdSave = cli.addCommand("save,w/rite", saveCallback);
  cmdErase = cli.addCommand("erase", eraseCallback);
  cmdDump = cli.addCommand("du/mp", dumpCallback);
  cmdWifi = cli.addBoundlessCommand("wifi", wifiCallback);
  cmdSSID = cli.addBoundlessCommand("ssid", ssidCallback);
  cmdPass = cli.addBoundlessCommand("pass", passCallback);
  cmdReboot = cli.addCommand("reboot", rebootCallback);
  cmdUpdate = cli.addCommand("update", updateCallback);
  cmdVersion = cli.addCommand("v/ersion", versionCallback);
  cmdType = cli.addBoundlessCommand("t/ype,cat", typeCallback);
  cmdExec = cli.addBoundlessCommand("e/xec,run", execCallback);

  dispPrompt();
}

void dispPrompt() {
  if (confChanged) {
    Serial.print("* ");
  }
  Serial.print(cliPrompt);
}

// Get input
void cliInput() {
  byte c;

  while (Serial.available()) {
    c = Serial.read();

    switch (c) {
      case '\n':
        Serial.printf("\r\n");
        cliIdx = 0;
        cli.parse(cliBuf);
        dispPrompt();
        cliBuf[0] = 0;
        return;

      case '\b':
        if (cliIdx) {
          Serial.print("\b \b");
          cliBuf[cliIdx--] = 0;
        }
        break;

      default:
        if (cliIdx >= sizeof(cliBuf)-2) {
          Serial.print("\a");
          return;
        }

        Serial.printf("%c", c);
        cliBuf[cliIdx++] = c;
        cliBuf[cliIdx] = 0;
        break;
    }
  }
}

void helpCallback(cmd* c) {
  Serial.printf("BAUD baud                 Set FDC+ baud rate\r\n");
  Serial.printf("DIR                       Directory\r\n");
  Serial.printf("DUMP                      Dump track buffer\r\n");
  Serial.printf("ERASE                     Erase configuration\r\n");
  Serial.printf("EXEC filename             Execute filename\r\n");
  Serial.printf("MOUNT [drive filename]    Mount drive\r\n");
  Serial.printf("PASS pass                 Set WiFi password\r\n");
  Serial.printf("REBOOT                    Reboot device\r\n");
  Serial.printf("SAVE                      Save configuration\r\n");
  Serial.printf("SSID ssid                 Set WiFi SSID\r\n");
  Serial.printf("STATS                     Statistics\r\n");
  Serial.printf("TYPE filename             Display file\r\n");
  Serial.printf("UNMOUNT drive             Unmount drive\r\n");
  Serial.printf("UPDATE                    Update firmware\r\n");
  Serial.printf("VERSION                   Dispay version\r\n");
  Serial.printf("WIFI ON | OFF             Turn WiFi On and Off\r\n");
}

void versionCallback(cmd * c) {
  Serial.printf("%d.%d\r\n", MAJORVER, MINORVER);
}

void rebootCallback(cmd* c) {
  Serial.printf("Rebooting...\r\n");
  delay(1000);
  ESP.restart();
}

void updateCallback(cmd* c) {
  Serial.printf("Not implemented\r\n");
}

void typeCallback(cmd* c) {
  Command cmd(c);
  char ch;

  int argNum = cmd.countArgs(); // Get number of arguments

  if (!argNum) {
    Serial.printf("type filename\r\n");
    return;
  }

  Argument filenameArg = cmd.getArg(0);
  String filename = "/" + filenameArg.getValue();

  File f = SD.open(filename);

  if (!f) {
    Serial.printf("Could not open file\r\n");
    return;
  }

  while(f.available()) {
    String s = f.readStringUntil('\n');
    Serial.print(s + "\r\n");
  }

  f.close();
}

void execCallback(cmd* c) {
  Command cmd(c);
  char ch;

  int argNum = cmd.countArgs(); // Get number of arguments

  if (!argNum) {
    Serial.printf("type filename\r\n");
    return;
  }

  Argument filenameArg = cmd.getArg(0);
  String filename = "/" + filenameArg.getValue();

  File f = SD.open(filename);

  if (!f) {
    f = SD.open(filename + ".bat");
    if (!f) {
      Serial.printf("Could not open file\r\n");
      return;
    }
  }

  while(f.available()) {
    cli.parse(f.readStringUntil('\n'));
  }

  f.close();
}

void baudCallback(cmd* c) {
  Command cmd(c);

  int argNum = cmd.countArgs(); // Get number of arguments

  if (argNum != 1) {
    Serial.printf("baud [9600, 19200, 38400, 57600, 76800, 230400, 403200, 460800]\r\n");
    return;
  }

  Argument driveArg = cmd.getArg(0);
  String driveValue = driveArg.getValue();
  baudRate = driveValue.toInt();

  fdcBaudrate();
}

// Callback function for dir command
void dirCallback(cmd* c) {
    Command cmd(c);               // Create wrapper object

    int argNum = cmd.countArgs(); // Get number of arguments

    listDir(SD, "/", 0);
}

// Callback function for mount command
void mountCallback(cmd* c) {
    Command cmd(c);               // Create wrapper object

    int argNum = cmd.countArgs(); // Get number of arguments

    // If no arguments, dump mount table
    if (!argNum) {
      for (int d=0; d<MAX_DRIVE; d++) {
        Serial.printf("Drive %d: ",d);
        if (drive[d].mounted) {
          Serial.printf("%-25.25s (%d Tracks)\r\n", drive[d].filename, drive[d].tracks);
        } else {
          Serial.printf("[ NOT MOUNTED ]\r\n");
        }
      }
      return;
    } else if (argNum != 2) {
      Serial.printf("mount [drive] [filename]\r\n");
      return;
    }

    Argument driveArg = cmd.getArg(0);
    String driveValue = driveArg.getValue();
    int d = driveValue.toInt();

    if (d<0 || d>MAX_DRIVE) {
      Serial.printf("Drive must be between 0 and %d\r\n", MAX_DRIVE);
      return;
    }
    Argument filenameArg = cmd.getArg(1);
    String filename = "/" + filenameArg.getValue();
    mountDrive(d,filename.c_str());
}

// Callback function for unmount command
void unmountCallback(cmd* c) {
    Command cmd(c);               // Create wrapper object

    int argNum = cmd.countArgs(); // Get number of arguments

     // If no arguments, dump mount table
    if (!argNum) {
      Serial.printf("unmount [drive]\r\n");
      return;
    }

    Argument driveArg = cmd.getArg(0);
    String driveValue = driveArg.getValue();
    int d = driveValue.toInt();

    if (d<0 || d>MAX_DRIVE) {
      Serial.printf("Drive must be between 0 and %d\r\n", MAX_DRIVE - 1);
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

  for (int d=0; d<MAX_DRIVE; d++) {
    sprintf(key, "Drive%d", d);
    fdcPrefs.putString(key, drive[d].filename);
  }

  Serial.printf("Configuration saved.\r\n");

  confChanged = false;
}

// Callback function for erase command
void eraseCallback(cmd* c) {
    nvs_flash_erase();      // erase the NVS partition and...
    nvs_flash_init();       // initialize the NVS partition.

    Serial.printf("Configuration erased.\r\n");

    confChanged = false;

    loadPrefs();
}

// Callback function for dump command
void dumpCallback(cmd* c) {
    Serial.printf("D:%02d T:%04d Track Buffer:\r\n", lastDrive, lastTrack);
    dumpBuffer(trackBuf, sizeof(trackBuf));
}

// Callback function for stats command
void statsCallback(cmd* c) {
    Command cmd(c);               // Create wrapper object

    int argNum = cmd.countArgs(); // Get number of arguments

    Serial.printf("FDC+ Baud Rate: %d\r\n\n", baudRate);

    Serial.printf("STAT: %08d\r\n", statCnt);
    Serial.printf("READ: %08d\r\n", readCnt);
    Serial.printf("WRIT: %08d\r\n", writCnt);
    Serial.printf("ERRS: %08d\r\n", errsCnt);
    Serial.printf("TOUT: %08d\r\n", toutCnt);

    Serial.printf("Last STAT : %s\r\n", lastStat);
    Serial.printf("Last READ : %s\r\n", lastRead);
    Serial.printf("Last WRIT : %s\r\n", lastWrit);
    Serial.printf("Last Error: %s\r\n", lastErr);
}

// Callback function for stats command
void wifiCallback(cmd* c) {
    Command cmd(c);               // Create wrapper object
    bool prevWifi = wifiEnabled;

    int argNum = cmd.countArgs(); // Get number of arguments

    if (!argNum) {
      Serial.printf("WiFi %s\n", (wifiEnabled) ? "Enabled" : "Disabled");
      Serial.printf("WiFi SSID: %s\r\n", (strlen(wifiSSID)) ? wifiSSID : "Not Set");
      Serial.printf("WiFi PASS: %s\r\n\n", (strlen(wifiPass)) ? "******" : "Not Set");
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
      Serial.printf("Invalid option\r\n");
      return;
    }

    if (wifiEnabled && !prevWifi){
      wifiSetup();
      confChanged = true;
    } else if (!wifiEnabled && prevWifi) {
      wifiDisconnect();
      confChanged = true;
    }
}

// Callback function for ssid command
void ssidCallback(cmd* c) {
    Command cmd(c);               // Create wrapper object

    int argNum = cmd.countArgs(); // Get number of arguments

    if (!argNum) {
      Serial.printf("ssid SSID\r\n");
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
    Command cmd(c);               // Create wrapper object

    int argNum = cmd.countArgs(); // Get number of arguments

    if (!argNum) {
      Serial.printf("pass PASSWORD\r\n");
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
    CommandError cmdError(e); // Create wrapper object
    Command cmd;

    Serial.print("ERROR: ");
    Serial.println(cmdError.toString());

    if (cmdError.hasCommand()) {
        Serial.print("Did you mean \"");
        Serial.print(cmdError.getCommand().toString());
        Serial.println("\"?");
    }
}