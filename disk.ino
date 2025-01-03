void diskSetup() {
//  listDir(SD, "/", 0);
}

bool mountDrive(int driveno, const char *filename) {
  if (driveno < 0 || driveno >= MAX_DRIVE) {
    cliConsole->printf("Drive %d: invalid drive number\r\n");
    return false;
  }
  
  // Try to initialize SD card if not ready
  if (!sdReady) {
    if (!sdSetup()) {
      return false;
    }
  }

  if (drive[driveno].mounted) {
    unmountDrive(driveno);
  }

  if (SD.exists(filename)) {
    drive[driveno].diskImg=SD.open(filename, "r+");

    if (!drive[driveno].diskImg) {
      cliConsole->printf("Drive %d: could not open '%s'\r\n", driveno, filename);
      return false;
    }

    drive[driveno].mounted = true;
    strncpy(drive[driveno].filename, filename, sizeof(drive[driveno].filename));

    cliConsole->printf("Drive %d: mounted as '%s's.\r\n", driveno, filename+1);

    lastDrive = -1;
    lastTrack = -1;

    confChanged = true;
  }
  else {
    cliConsole->printf("File '%s' does not exist\r\n", filename);
  }

  return true;
}

void unmountDrive(int driveno) {
  if (driveno >= 0 && driveno < MAX_DRIVE) {
    if (drive[driveno].mounted || drive[driveno].diskImg) {
      drive[driveno].diskImg.close();
      drive[driveno].mounted = 0;
      drive[driveno].filename[0]= 0;
      cliConsole->printf("Drive %d: unmounted\r\n", driveno);

      lastDrive = -1;
      lastTrack = -1;

      confChanged = true;
    }
  }
}

void listDir(fs::SDFS &fs, const char * dirname, uint8_t levels){
  const char *f;

  // Try to initialize SD card if not ready
  if (!sdReady) {
    if (!sdSetup()) {
      return;
    }
  }

  cliConsole->printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    cliConsole->printf("Failed to open directory\r\n");
    return;
  }
  if(!root.isDirectory()){
    cliConsole->printf("Not a directory\r\n");
    return;
  }

  File file = root.openNextFile();
  while(file) {
    f = file.name();
    if (*f != '.') {
      cliConsole->printf("%-25.25s %8d\r\n", f, file.size());
    }
    file = root.openNextFile();
  }

  root.close();
}