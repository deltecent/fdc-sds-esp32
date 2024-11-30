/*----------------------------------------------------------------------------------
;
;  Altair FDC+ Serial Disk Server
;      This program serves Altair disk images over a high speed serial port
;      for computers running the FDC+ Enhanced Floppy Disk Controller.
;
;     Version     Date        Author         Notes
;      1.0     11/03/2024     P. Linstruth   Original
;
;-------------------------------------------------------------------------------------
;
;  Communication with the server is over a serial port at 403.2K Baud, 8N1.
;  All transactions are initiated by the FDC. The second choice for baud rate
;  is 460.8K. Finally, 230.4K is the most likely supported baud rate on the PC
;  if 403.2K and 460.8K aren't avaialable.
;
;  403.2K is the preferred rate as it allows full-speed operation and is the
;  most accurate of the three baud rate choices on the FDC. 460.8K also allows
;  full speed operation, but the baud rate is off by about 3.5%. This works, but
;  is borderline. 230.4K is available on most all serial ports, is within 2% of
;  the FDC baud rate, but runs at 80%-90% of real disk speed.
;
;  FDC TO SERVER COMMANDS
;    Commands from the FDC to the server are fixed length, ten byte messages. The 
;    first four bytes are a command in ASCII, the remaining six bytes are grouped
;    as three 16 bit words (little endian). The checksum is the 16 bit sum of the
;    first eight bytes of the message.
;
;    Bytes 0-3   Bytes 4-5 as Word   Bytes 6-7 as Word   Bytes 8-9 as Word
;    ---------   -----------------   -----------------   -----------------
;     Command       Parameter 1         Parameter 2           Checksum
;
;    Commands:
;      STAT - Provide and request drive status. The FDC sends the selected drive
;             number and head load status in Parameter 1 and the current track 
;             number in Parameter 2. The Server responds with drive mount status
;             (see below). The LSB of Parameter 1 contains the currently selected
;             drive number or 0xff is no drive is selected. The MSB of parameter 1
;             is non-zero if the head is loaded, zero if not loaded.
;
;             The FDC issues the STAT command about ten times per second so that
;             head status and track number information is updated quickly. The 
;             server may also want to assume the drive is selected, the head is
;             loaded, and update the track number whenever a READ is received.
;
;      READ - Read specified track. Parameter 1 contains the drive number in the
;             MSNibble. The lower 12 bits contain the track number. Transfer length
;             length is in Parameter 2 and must be the track length. Also see
;             "Transfer of Track Data" below.
;
;      WRIT - Write specified track. Parameter 1 contains the drive number in the
;             MSNibble. The lower 12 bits contain the track number. Transfer length
;             must be track length. Server responds with WRIT response when ready
;             for the FDC to send the track of data. See "Transfer of Track Data" below.
;
;
;  SERVER TO FDC 
;    Reponses from the server to the FDC are fixed length, ten byte messages. The 
;    first four bytes are a response command in ASCII, the remaining six bytes are
;    grouped as three 16 bit words (little endian). The checksum is the 16 bit sum
;    of the first eight bytes of the message.
;
;    Bytes 0-3   Bytes 4-5 as Word   Bytes 6-7 as Word   Bytes 8-9 as Word
;    ---------   -----------------   -----------------   -----------------
;     Command      Response Code        Reponse Data          Checksum
;
;    Commands:
;      STAT - Returns drive status in Response Data with one bit per drive. "1" means a
;             drive image is mounted, "0" means not mounted. Bits 15-0 correspond to
;             drive numbers 15-0. Response code is ignored by the FDC.
;
;      WRIT - Issued in repsonse to a WRIT command from the FDC. This response is
;             used to tell the FDC that the server is ready to accept continuous transfer
;             of a full track of data (response code word set to "OK." If the request
;             can't be fulfilled (e.g., specified drive not mounted), the reponse code
;             is set to NOT READY. The Response Data word is don't care.
;
;      WSTA - Final status of the write command after receiving the track data is returned
;             in the repsonse code field. The Response Data word is don't care.
;
;    Reponse Code:
;      0x0000 - OK
;      0x0001 - Not Ready (e.g., write request to unmounted drive)
;      0x0002 - Checksum error (e.g., on the block of write data)
;      0x0003 - Write error (e.g., write to disk failed)
;
;
;  TRANSFER OF TRACK DATA
;    Track data is sent as a sequence of bytes followed by a 16 bit, little endian 
;    checksum. Note the Transfer Length field does NOT include the two bytes of
;    the checksum. The following notes apply to both the FDC and the server.
;
;  ERROR RECOVERY
;    The FDC uses a timeout of one second after the last byte of a message or data block
;        is sent to determine if a transmission was ignored.
;
;    The server should ignore commands with an invalid checksum. The FDC may retry the
;        command if no response is received. An invalid checksum on a block of write
;        data should not be ignored, instead, the WRIT response should have the
;        Reponse Code field set to 0x002, checksum error.
;
;    The FDC ignores responses with an invalid checksum. The FDC may retry the command
;        that generated the response by sending the command again.
;  
----------------------------------------------------------------------------------*/

#define RESP_OK 0
#define RESP_NOT_READY 1
#define RESP_CHECKSUM_ERR 2
#define RESP_WRITE_ERR 3

#define CMD_STAT  1
#define CMD_READ  2
#define CMD_WRIT  3

#define WORD(lsb, msb) ((unsigned short) ((msb << 8) + lsb))

void ARDUINO_ISR_ATTR fdcTimerISR() {
  fdcTimeout = true;

  // Turn off STAT LED
  digitalWrite(LED_BUILTIN, false);
}

void fdcSetup() {

  // Drive select LED GPIO
  for (int d=0; d<MAX_DRIVE; d++) {
    pinMode(dSelLED[d], OUTPUT);
    digitalWrite (dSelLED[d], HIGH);
    delay(250);	// wait for half a second or 500 milliseconds
    digitalWrite (dSelLED[d], LOW);
  }

  // Start UART with baud rate above 250K to set up APB clock
  fdcSerial.begin(baudRate, SERIAL_8N1, 16, 17);
  fdcBaudrate();
}

void fdcBaudrate() {
  switch (baudRate) {
    case 9600:
    case 19200:
    case 38400:
    case 57600:
    case 76800:
    case 115200:
    case 230400:
    case 403200:
    case 460800:
      break;

    default:
      break;
      baudRate = DEFAULT_BAUD;
      cliConsole->printf("Invalid baud rate. Resetting FDC+ to default baud rate %d.\r\n", baudRate);
      break;
  }

  //fdcSerial.updateBaudRate(baudRate);
  fdcSerial.end();
  fdcSerial.begin(baudRate, SERIAL_8N1, 16, 17);

//  if (fdcSerial.baudRate() != baudRate) {
//    cliConsole->printf("Could not set baud rate to %d. Returned %d.\r\n", baudRate, fdcSerial.baudRate());
//  }
}

bool fdcProc(void) {
  crblk_t cmd;

  if (!fdcSerial.available()) {
    return false;
  }

  if (recvBlock(cmd.block, sizeof(cmd), 1000) == false) {
    return false;
  }

  // Turn on status LED, reset timer
  fdcTimeout = false;
  timerRestart(fdcTimer);
  digitalWrite(LED_BUILTIN, true);

  if (memcmp(cmd.cmd, "STAT", 4) == 0) {
    statCnt++;

    procSTAT(&cmd);
  }
  else if (memcmp(cmd.cmd, "READ", 4) == 0) {
    readCnt++;

    procREAD(&cmd);
  }
  else if (memcmp(cmd.cmd, "WRIT", 4) == 0) {
    writCnt++;

    procWRIT(&cmd);
  }
  else {
    strcpy(lastErr, "Unknown Command");
    return false;
  }

  return true;
}

int procSTAT(crblk_t *cmd) {
  // Send STAT response
  cmd->word2[LSB] = 0x00;
  cmd->word2[MSB] = 0x00;

  for (int d=0; d<MAX_DRIVE; d++) {
    digitalWrite(dSelLED[d], LOW);

    if (d < 8) {
      cmd->word2[LSB] |= ((drive[d].mounted) ? 1 << d : 0);
    } else {
      cmd->word2[MSB] |= ((drive[d].mounted) ? 1 << (d - 8) : 0);
    }
  }

  // Set drive select and head load LED
  if ((cmd->word1[LSB] < MAX_DRIVE) && cmd->word1[MSB]) {
    digitalWrite(dSelLED[cmd->word1[LSB]], HIGH);
  }

  sprintf(lastStat, "%02X %02X %02X %02X", cmd->word1[LSB], cmd->word1[MSB], cmd->word2[LSB], cmd->word2[MSB]);

  return sendBlock(cmd->block, sizeof(cmd->block), false, 5000);
}

bool procREAD(crblk_t *cmd) {
  byte d;
  int track;
  int len;
  byte checksum[2];

  d = cmd->word1[MSB] >> 4;
  track = ((cmd->word1[MSB] & 0x0f) << 8) + cmd->word1[LSB];
  len = (cmd->word2[MSB] << 8) + cmd->word2[LSB];
 
  sprintf(lastRead, "D:%02d T:%04d L:%d", d, track, len);

  if (d >= MAX_DRIVE) {
    cliConsole->printf("Drive %d: Invalid drive number\r\n", d);
    return false;
  }

  if (track >= drive[d].tracks) {
    cliConsole->printf("Drive %d: invalid track %d\r\n", d, track);
    return false;
  }
  
  if (len > TRACKSIZE) {
    cliConsole->printf("Drive %d: requested length %d exceeds buffer size %d\r\n", d, len, TRACKSIZE);
    return false;
  }

  if (!drive[d].mounted) {
    cliConsole->printf("Drive %d: not mounted\r\n", d);
    return false;
  }

  if (lastDrive != d || lastTrack != track) {
    File diskImg;
    diskImg=SD.open(drive[d].filename);

    if (!diskImg) {
      cliConsole->printf("Drive %d: could not open '%s'\r\n", d, "filename");
      return false;
    }

    diskImg.seek(track * len);
    
    int bytesRead = diskImg.read(trackBuf, len);

    diskImg.close();
    
    if (bytesRead != len) {
      cliConsole->printf("Drive %d: Could not read %d bytes\r\n", d, len);
      return false;
    }

    lastDrive = d;
    lastTrack = track;
  }

  return sendBlock(trackBuf, len, false, 5000);
}

bool procWRIT(crblk_t *cmd) {
  byte d;
  int track;
  int len;
  int bytesWritten;
  byte checksum[2];

  d = cmd->word1[MSB] >> 4;
  track = ((cmd->word1[MSB] & 0x0f) << 8) + cmd->word1[LSB];
  len = (cmd->word2[MSB] << 8) + cmd->word2[LSB];
 
  sprintf(lastWrit, "D:%02d T:%04d L:%d", d, track, len);

  if (d >= MAX_DRIVE) {
    cliConsole->printf("Drive %d: Invalid drive number\r\n", d);
    return false;
  }

  if (track >= drive[d].tracks) {
    cliConsole->printf("Drive %d: invalid track %d\r\n", d, track);
    return false;
  }
  
  if (len > TRACKSIZE) {
    cliConsole->printf("Drive %d: requested length %d exceeds buffer size %d\r\n", d, len, TRACKSIZE);
    return false;
  }

  File diskImg=SD.open(drive[d].filename, "r+");

  // Clear response MSB
  cmd->word1[MSB] = 0x00;

  if (!diskImg) {
    cliConsole->printf("Drive %d: could not open '%s'\r\n", d, drive[d].filename);
    // Send WRIT response
    cmd->word1[LSB] = RESP_NOT_READY;

    sendBlock(cmd->block, sizeof(cmd->block), false, 1000);
     return false;
  }

  // Let FDC+ know we're ready to receive the track
  cmd->word1[LSB] = RESP_OK;

  sendBlock(cmd->block, sizeof(cmd->block), false, 1000);

  // Wait for track
  if (recvBlock(trackBuf, len, 5000) == false) {
    sprintf(lastErr, "Timeout waiting for block %d bytes\r\n", len);
    diskImg.close();
    return false;
  }

  // Seek to track
  diskImg.seek(track * len);

  if (diskImg.position() != track * len) {
    sprintf(lastErr, "Drive %d: seek to %d failed\r\n", d, track * len);
  } else {
   bytesWritten = diskImg.write(trackBuf, len);
  }

  diskImg.close();

  // Send write status
  memcpy(cmd->cmd, "WSTA", 4);

  if (bytesWritten != len) {
    sprintf(lastErr, "Drive %d: Could not write %d bytes, wrote %d\r\n", d, len, bytesWritten);
    cmd->word1[LSB] = RESP_WRITE_ERR;

    sendBlock(cmd->block, sizeof(cmd->block), false, 1000);
    //dumpBuffer(cmd->block, sizeof(cmd->block));
    return false;
  }

  cmd->word1[LSB] = RESP_OK;
  sendBlock(cmd->block, sizeof(cmd->block), false, 1000);

  lastDrive = d;
  lastTrack = track;

  return true;
}

unsigned short calcChecksum(byte *buffer, int len) {
  unsigned int checksum = 0;

  while (len--) {
    checksum += *buffer;
    buffer++;
  }

  return checksum & 0xffff;
}

unsigned short setChecksum(byte *buffer, byte *checksum, int len) {
  unsigned short calc = 0;

  calc = calcChecksum(buffer, len);

  checksum[0] = calc & 0x00ff;
  checksum[1] = calc >> 8;

  return calc;
}

bool sendBlock(byte *block, int len, bool hasChecksum, unsigned long timeout) {
  byte checksum[2];

  fdcSerial.setTimeout(timeout);

  if (fdcSerial.write(block, len) != len) {
    sprintf(lastErr, "Could not send %d bytes\r\n", len);
    return false;
  }

  // Is the checksum included in the block?
  if (hasChecksum) {
    return true;
  }

  setChecksum(block, checksum, len);

  return fdcSerial.write(checksum, sizeof(checksum)) == sizeof(checksum);
}

bool recvBlock(byte *block, int len, unsigned long timeout) {
  byte checksum[2];
  unsigned short calc;
  int rcvd;

  // Receive data
  fdcSerial.setTimeout(timeout);
  if ((rcvd = fdcSerial.readBytes(block, len)) != len) {
      sprintf(lastErr, "BLOCK TIMEOUT - Received %d of %d bytes\n", rcvd, len);
      toutCnt++;
      flushrx(&fdcSerial);
      return false;
  }

  // Receive checksum
  if ((rcvd = fdcSerial.readBytes(checksum, sizeof(checksum))) != sizeof(checksum)) {
      sprintf(lastErr, "CHECKSUM TIMEOUT - Received %d of %d bytes\n", rcvd, len);
      flushrx(&fdcSerial);
      return false;
  }

  calc = calcChecksum(block, len);

  if (WORD(checksum[LSB], checksum[MSB]) == calc) {
    return true;
  }

  errsCnt++;

  sprintf(lastErr, "Checksum error receiving %d byte block", len);
  flushrx(&fdcSerial);

  return false;
}

void dumpBuffer(byte *buffer, int len) {
  for (int i=0; i<len; i++) {
    if (i % 16 == 0) {
      cliConsole->printf("\r\n%04X: ", i);
    }
    cliConsole->printf("%02X ", buffer[i]);
  }
  cliConsole->println("");
}