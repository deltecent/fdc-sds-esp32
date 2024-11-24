
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
  WiFi.setAutoReconnect(true);
  WiFi.onEvent(wifiEvent);

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

// WARNING: This function is called from a separate FreeRTOS task (thread)!
void wifiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\r\n", event);

  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:               Serial.println("WiFi interface ready"); break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:           Serial.println("Completed scan for access points"); break;
    case ARDUINO_EVENT_WIFI_STA_START:           Serial.println("WiFi client started"); break;
    case ARDUINO_EVENT_WIFI_STA_STOP:            Serial.println("WiFi clients stopped"); break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:       Serial.println("Connected to access point"); break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:    Serial.println("Disconnected from WiFi access point"); break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: Serial.println("Authentication mode of access point has changed"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
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
  cliConsole = &TelnetStream;
  cliConsole->printf("\r\nESP32 FDC+ Serial Drive Server %d.%d\r\n\r\n", MAJORVER, MINORVER);
  dispPrompt();

  Serial.printf("\r\nTelnet: %s connected.\r\n", ip.c_str());
}

void telnetDisconnected(String ip) {
  Serial.printf("\r\nTelnet: %s disconnected.\r\n", ip.c_str());
}