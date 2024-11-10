void diskSetup() {
  listDir(SD, "/", 0);  
}

int diskTracks(const char *filename) {
  int tracks;

  File disk = SD.open(filename);

  if (!disk) {
    Serial.printf("Could not open '%s'\r\n", filename);
  }

  tracks = disk.size() / TRACKSIZE;

  disk.close();

  return tracks;
}

void mountDrive(int driveno, const char *filename) {
  if (driveno < 0 || driveno >= MAX_DRIVE) {
    Serial.printf("Drive %d: invalid drive number\r\n");
    return;
  }
  
  if (drive[driveno].mounted) {
    unmountDrive(driveno);
  }

  if (SD.exists(filename)) {
    drive[driveno].mounted = true;
    strncpy(drive[driveno].filename, filename, sizeof(drive[driveno].filename));
    drive[driveno].tracks = diskTracks(filename);

    Serial.printf("Drive %d: mounted as '%s' with %d tracks.\r\n", driveno, filename, drive[driveno].tracks);

    confChanged = true;
  }
  else {
    Serial.printf("File '%s' does not exist\r\n", filename);
  }
}

void unmountDrive(int driveno) {
  if (driveno >= 0 && driveno < MAX_DRIVE) {
    if (drive[driveno].mounted) {
      drive[driveno].mounted = 0;
      drive[driveno].filename[0]= 0;
      Serial.printf("Drive %d: unmounted\r\n", driveno);

      confChanged = true;
    }
  }
}

void listDir(fs::SDFS &fs, const char * dirname, uint8_t levels){
  const char *f;

  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    f = file.name();
    if (*f != '.') {
      Serial.printf("%-25.25s %8d\r\n", f, file.size());
    }
    file = root.openNextFile();
  }

  root.close();
}