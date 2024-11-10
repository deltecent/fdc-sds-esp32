
void wifiSetup() {

  if (!wifiEnabled) {
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

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);
    if (timeout++ > 40) {
      Serial.printf(" NOT CONNECTED\r\n");
      return;
    }
  }

  Serial.print(" CONNECTED ");
  Serial.print(WiFi.localIP());

  Serial.print("\r\n");
}

void wifiDisconnect() {

  WiFi.disconnect();

  Serial.printf("WiFi disconnected from '%s'.\r\n", wifiSSID);
}
