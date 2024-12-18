void diskSetup() {
//  listDir(SD, "/", 0);
}

int diskTracks(const char *filename) {
  int tracks;

  File disk = SD.open(filename);

  if (!disk) {
    cliConsole->printf("Could not open '%s'\r\n", filename);
  }

  tracks = disk.size() / TRACKSIZE;

  disk.close();

  return tracks;
}

void mountDrive(int driveno, const char *filename) {
  if (driveno < 0 || driveno >= MAX_DRIVE) {
    cliConsole->printf("Drive %d: invalid drive number\r\n");
    return;
  }
  
  if (drive[driveno].mounted) {
    unmountDrive(driveno);
  }

  if (SD.exists(filename)) {
    drive[driveno].mounted = true;
    strncpy(drive[driveno].filename, filename, sizeof(drive[driveno].filename));
    drive[driveno].tracks = diskTracks(filename);

    cliConsole->printf("Drive %d: mounted as '%s' with %d tracks.\r\n", driveno, filename+1, drive[driveno].tracks);

    lastDrive = -1;
    lastTrack = -1;

    confChanged = true;
  }
  else {
    cliConsole->printf("File '%s' does not exist\r\n", filename);
  }
}

void unmountDrive(int driveno) {
  if (driveno >= 0 && driveno < MAX_DRIVE) {
    if (drive[driveno].mounted) {
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
  while(file){
    f = file.name();
    if (*f != '.') {
      cliConsole->printf("%-25.25s %8d\r\n", f, file.size());
    }
    file = root.openNextFile();
  }

  root.close();
}