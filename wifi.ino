
void wifiSetup() {

  if (!wifiEnabled) {
    WiFi.disconnect(true);
    return;
  }

  if (!strlen(wifiSSID)) {
    Serial.printf("WiFi SSID not set.\r\n");
    return;
  }
  
  WiFi.setHostname("ESP32-FDC-SDS");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID, wifiPass);

  Serial.printf("WiFi connecting to '%s' ..", wifiSSID);

  int timeout = 0;
  int status;

  while ((status = WiFi.status()) != WL_CONNECTED) {
    Serial.print(".");

    delay(500);
    if (timeout++ > 40) {
      Serial.printf(" NOT CONNECTED\r\n");
      WiFi.disconnect();
      return;
    }
  }

  Serial.print(" CONNECTED ");
  Serial.print(WiFi.localIP());

  Serial.print("\r\n");

  // Start telnet console
  TelnetStream.begin();
  TelnetStream.onConnect(telnetConnected);
  TelnetStream.onDisconnect(telnetDisconnected);

  // Start FTP server
  ftpSrv.begin("fdc","fdc");    //username, password for ftp.
}

void wifiDisconnect() {
  ftpSrv.end();
  TelnetStream.stop();

  if (WiFi.isConnected()) {
    Serial.printf("WiFi disconnecting from '%s'.\r\n", wifiSSID);
  }

  // Disconnect and turn off radio
  WiFi.disconnect(true);
}

void telnetConnected(String ip) {
  cliConsole = &TelnetStream;
  cliConsole->printf("\r\nESP32 FDC+ Serial Drive Server %d.%d\r\n\r\n", MAJORVER, MINORVER);
  dispPrompt();

  Serial.printf("\r\nTelnet: %s connected.\r\n", ip.c_str());
}

void telnetDisconnected(String ip) {
  Serial.printf("\r\nTelnet: %s disconnected.\r\n", ip.c_str());
}