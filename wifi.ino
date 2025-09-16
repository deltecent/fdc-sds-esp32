
void wifiSetup() {

  if (!wifiEnabled || WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect(true);  // Will automatically reconnect
    return;
  }

  if (!strlen(wifiSSID)) {
    cliConsole->printf("WiFi SSID not set.\r\n");
    wifiEnabled = false;
    return;
  }

  if (!strlen(wifiPass)) {
    cliConsole->printf("WiFi PASSWORD not set.\r\n");
    wifiEnabled = false;
    return;
  }
  
  WiFi.mode(WIFI_MODE_NULL);
  WiFi.setHostname(wifiName);
  WiFi.mode(WIFI_STA);

  WiFi.removeEvent(wifiEvent);
  WiFi.onEvent(wifiEvent);

  // Add Wi-Fi network with SSID and Password
  wifiMulti.APlistClean();
  wifiMulti.addAP(wifiSSID, wifiPass);

  cliConsole->printf("WiFi connecting to '%s'\r\n", wifiSSID);

  int tries = 5;

  while (wifiMulti.run() != WL_CONNECTED && tries--) {
    delay(500);
  }
  //WiFi.begin(wifiSSID, wifiPass);

  //cliConsole->printf("WiFi connecting to '%s'\r\n", wifiSSID);
}

void wifiConnected() {
  // Start telnet console
  if (telnet.begin(23, false)) {
    telnet.onConnect(telnetConnected);
    telnet.onDisconnect(telnetDisconnected);
  }
  else {
    cliConsole->printf("Could not start telnet server\r\n");
  }

  // Start FTP server
  ftpSrv.begin("fdc","fdc");    //username, password for ftp.
}

void wifiDisconnected() {
  ftpSrv.end();
  telnet.stop();
}

void setClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // UTC

  cliConsole->print(F("Waiting for NTP time sync: "));
  time_t now = time(nullptr);
  int tries = 20;
  while ((now < 8 * 3600 * 2) && tries--) {
    yield();
    delay(500);
    cliConsole->print(F("."));
    now = time(nullptr);
  }

  cliConsole->printf(F("\r\n"));
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  cliConsole->print(F("Current time: "));
  cliConsole->printf("%.24s\r\n", F(asctime(&timeinfo)));
}

// WARNING: This function is called from a separate FreeRTOS task (thread)!
void wifiEvent(WiFiEvent_t event) {

  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:               Serial.println("WiFi interface ready"); break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:           Serial.println("Completed scan for access points"); break;
    case ARDUINO_EVENT_WIFI_STA_START:           Serial.println("WiFi client started"); break;
    case ARDUINO_EVENT_WIFI_STA_STOP:            Serial.println("WiFi clients stopped"); break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("Connected to access point");
      wifiConnected();
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("Disconnected from WiFi access point");
      wifiDisconnected();
      break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: Serial.println("Authentication mode of access point has changed"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      cliConsole->print("Obtained IP address: ");
      cliConsole->println(WiFi.localIP());
      // setClock();
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:        Serial.println("Lost IP address and IP address is reset to 0"); break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:          Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_FAILED:           Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:          Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_PIN:              Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode"); break;
    case ARDUINO_EVENT_WIFI_AP_START:           Serial.println("WiFi access point started"); break;
    case ARDUINO_EVENT_WIFI_AP_STOP:            Serial.println("WiFi access point  stopped"); break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:    Serial.println("Client connected"); break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: Serial.println("Client disconnected"); break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:   Serial.println("Assigned IP address to client"); break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:  Serial.println("Received probe request"); break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:         Serial.println("AP IPv6 is preferred"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:        Serial.println("STA IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_GOT_IP6:             Serial.println("Ethernet IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_START:               Serial.println("Ethernet started"); break;
    case ARDUINO_EVENT_ETH_STOP:                Serial.println("Ethernet stopped"); break;
    case ARDUINO_EVENT_ETH_CONNECTED:           Serial.println("Ethernet connected"); break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:        Serial.println("Ethernet disconnected"); break;
    case ARDUINO_EVENT_ETH_GOT_IP:              Serial.println("Obtained IP address"); break;
    default:                                    break;
  }
}

void telnetConnected(String ip) {
  cliConsole = &telnet;
  cliConsole->printf("\r\nESP32 FDC+ Serial Drive Server %d.%d\r\n\r\n", MAJORVER, MINORVER);
  dispPrompt();

  Serial.printf("\r\nTelnet: %s connected.\r\n", ip.c_str());
}

void telnetDisconnected(String ip) {
  Serial.printf("\r\nTelnet: %s disconnected.\r\n", ip.c_str());
}