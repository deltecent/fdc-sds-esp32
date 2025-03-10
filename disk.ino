void diskSetup() {
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

  if (!(SD.exists(filename))) {
    cliConsole->printf("File '%s' does not exist\r\n", filename);
    
    return false;
  }

  drive[driveno].diskImg=SD.open(filename, "r+");

  if (!drive[driveno].diskImg) {
    cliConsole->printf("Drive %d: could not open '%s'\r\n", driveno, filename);
    return false;
  }

  drive[driveno].mounted = true;
  drive[driveno].size = drive[driveno].diskImg.size();
  strncpy(drive[driveno].filename, filename, sizeof(drive[driveno].filename));

  cliConsole->printf("Drive %d: mounted as '%s'.\r\n", driveno, filename+1);

  lastDrive = -1;
  lastTrack = -1;

  return true;
}

bool unmountDrive(int driveno) {
  if (driveno >= 0 && driveno < MAX_DRIVE) {
    if (drive[driveno].mounted || drive[driveno].diskImg) {
      drive[driveno].diskImg.close();
      drive[driveno].mounted = 0;
      drive[driveno].filename[0]= 0;
      cliConsole->printf("Drive %d: unmounted\r\n", driveno);

      lastDrive = -1;
      lastTrack = -1;

      return true;
    }
  }

  return false;
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
      time_t modTime = file.getLastWrite();
      cliConsole->printf("%-32.32s %8d\r\n", f, file.size());
    }
    file.close();
    file = root.openNextFile();
  }

  root.close();
}