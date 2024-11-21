/*
 * 
 * You can use this simple file (no external library dependancies) to flash your ESP32. It will then download and install the HemmTree program from GitHub
 * Just change the ssid and password below to your network
 * 
 */
 
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <EEPROM.h>

WiFiClient espClient;

const char* ssid = "HemmTree Setup"; 
const char* password = "12345678";

#define URL_fw_Bin "https://raw.githubusercontent.com/NextGenTechBar/HemmTree/main/ESP32_code.bin"

#define EEPROM_SIZE 12

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
  Serial.println();
  Serial.print("Mac Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Connecting to ");
  Serial.println(ssid);

  EEPROM.begin(EEPROM_SIZE);

  //Write data into eeprom
  int address = 0;
  int boardId = 18;
  EEPROM.write(address, boardId);//EEPROM.put(address, boardId);
  address += sizeof(boardId); //update address value

  float param = 46; // CHANGE THIS VALUE TO WHAT YOU WANT THE STRING LENGTH TO BEEEEEEEEEEEEEEEEEEEEEEEE
  EEPROM.writeFloat(address, param);//EEPROM.put(address, param);
  EEPROM.commit();
  EEPROM.end();

  //WiFi.persistent(false); //don't save configuration to flash, otherwise it won't start a captive portal which lets the user set the strip length on next boot of real firmware
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("successfully connected to wifi.");
  Serial.println("Will now attempt to download new firmware...");

  WiFiClientSecure client;
  //WiFiClientSecure * client = new WiFiClientSecure;
  //client.setCACert(rootCACertificate);
  client.setInsecure(); //prevents having the update the CA certificate periodically (it expiring breaks github updates which SUCKS cause you have to update each ornament manually with the new certificate
  httpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    Serial.println("This may indicate this device is stuck in a captive portal");
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
