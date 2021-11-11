/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <WiFi.h>
#include <PubSubClient.h>

//this block for GITHUB update
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "cert.h"

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 33
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(150, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(); //length, etc is read from eeprom and set in setup()


// Replace the next variables with your SSID/Password combination
const char* ssid = "Gonzaga Guest"; //CenturyLink3314
const char* password = ""; //buet2kpjnnbtw9
const char * deviceMacAddress = WiFi.macAddress().c_str();

// Add your MQTT Broker IP address, example:
//const char* mqtt_server = "192.168.1.144";
const char* mqtt_server = "broker.mqtt-dashboard.com";

#include <EEPROM.h>//https://github.com/espressif/arduino-esp32/tree/master/libraries/EEPROM

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

//uncomment the following lines if you're using SPI
/*#include <SPI.h>
#define BME_SCK 18
#define BME_MISO 19
#define BME_MOSI 23
#define BME_CS 5*/


// LED Pin
const int ledPin = 4;

//GITHUB update code. Change this number for each version increment
String FirmwareVer = {
  "0.01"
};
#define URL_fw_Version "https://raw.githubusercontent.com/NextGenTechBar/HemmTree/main/code_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/NextGenTechBar/HemmTree/main/ESP32_code.bin"

void firmwareUpdate();
int FirmwareVersionCheck();

void setup() {
  Serial.begin(115200);

  //neopixel setup
    // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code


  //Read data from eeprom to set number of pixels
  int stripLength=0;
  int address = 0;
  int readId;
  EEPROM.begin(12); //EEPROM size
  readId = EEPROM.read(address); //EEPROM.get(address,readId);
  //Serial.print("Read Id = ");
  //Serial.println(readId);
  address += sizeof(readId); //update address value

  float readParam;
  EEPROM.get(address, readParam); //readParam=EEPROM.readFloat(address);
  //Serial.print("VALUE STORED IN EEPROM = ");
  //Serial.println(readParam);
  if(isnan(readParam)){
    stripLength=150; //default length
    Serial.print("No value for strip length stored in EEPROM. Using default value of ");
    Serial.println(stripLength);
  }else{
    stripLength=(int)readParam;
    Serial.print("Setting strip length to ");
    Serial.print(stripLength);
    Serial.println(" based on EEPROM value");
  }
  Serial.println("Run EEPROM value update file on GITHUB to change strip length on this ESP");

  EEPROM.end();
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(150, PIN, NEO_GRB + NEO_KHZ800);
  strip.updateType(NEO_GRB + NEO_KHZ800);
  strip.setPin(PIN);
  strip.updateLength(stripLength);


  strip.begin();
  strip.setBrightness(255);
  strip.show(); // Initialize all pixels to 'off'
  //----
  
  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  //status = bme.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.print("Active firmware version:");
  Serial.println(FirmwareVer);
  Serial.println("Will now check for new firmware..");
  if (FirmwareVersionCheck()) {
      firmwareUpdate();
    }

  pinMode(ledPin, OUTPUT);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  //initialize blank array to store colors in (eventually pushed to strip)
  int stringUpdate[strip.numPixels()][3];
  for(int i=0;i<strip.numPixels();i++){
    for(int k=0;k<3;k++){
      stringUpdate[i][k]=0;
    }
  }

  if(messageTemp=="FIRMWARE_UPDATE"){
  Serial.println("Received instruction to update firmware.");
  Serial.print("Active firmware version:");
  Serial.println(FirmwareVer);
  Serial.println("Will now check for new firmware..");
  if (FirmwareVersionCheck()) {
      firmwareUpdate();
    }
  }

  int repetitions=0;
  if(messageTemp.substring(0,5)=="COLOR"){ //if it's a raw color
    int msgLen=9;
    for(int i=0;i<strip.numPixels();i++){
      stringUpdate[i][0]=messageTemp.substring(5+i*msgLen,8+i*msgLen).toInt(); //red
      stringUpdate[i][1]=messageTemp.substring(8+i*msgLen,11+i*msgLen).toInt(); //green
      stringUpdate[i][2]=messageTemp.substring(11+i*msgLen,14+i*msgLen).toInt(); //blue
      
      if(messageTemp[14+i*msgLen]==NULL){
        repetitions=i+1;
        break;
      }
    }

    //now fill the rest of the empty string with a repetition of the beginning
    for(int i=repetitions;i<strip.numPixels();i++){
      stringUpdate[i][0]=stringUpdate[i%repetitions][0];
      stringUpdate[i][1]=stringUpdate[i%repetitions][1];
      stringUpdate[i][2]=stringUpdate[i%repetitions][2];
    }

    for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(stringUpdate[i][0],stringUpdate[i][1],stringUpdate[i][2]));
    }
    strip.show();


  for(int i=0;i<strip.numPixels();i++){
    for(int k=0;k<3;k++){
      Serial.print(stringUpdate[i][k]);
      Serial.print(" ");
    }
    Serial.println();
  }
  }

  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(deviceMacAddress)) { //make client ID the mac address to ensure it's unique
      Serial.println("connected");
      // Subscribe
      client.subscribe("python/testing");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


    delay(2000);
    Serial.println("animating1...");
    client.loop();
    delay(2000);
    Serial.println("animating2...");
  

}



//GITHUB FIRMWARE UPDATE
void firmwareUpdate(void) {
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  httpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}
int FirmwareVersionCheck(void) {
  String payload;
  int httpCode;
  String fwurl = "";
  fwurl += URL_fw_Version;
  fwurl += "?";
  fwurl += String(rand());
  Serial.println(fwurl);
  WiFiClientSecure * client = new WiFiClientSecure;

  if (client) 
  {
    client -> setCACert(rootCACertificate);

    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
    HTTPClient https;

    if (https.begin( * client, fwurl)) 
    { // HTTPS      
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      delay(100);
      httpCode = https.GET();
      delay(100);
      if (httpCode == HTTP_CODE_OK) // if version received
      {
        payload = https.getString(); // save received version
      } else {
        Serial.print("error in downloading version file:");
        Serial.println(httpCode);
      }
      https.end();
    }
    delete client;
  }
      
  if (httpCode == HTTP_CODE_OK) // if version received
  {
    payload.trim();
    if (payload.equals(FirmwareVer)) {
      Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
      return 0;
    } 
    else 
    {
      Serial.println(payload);
      Serial.println("New firmware detected");
      return 1;
    }
  } 
  return 0;  
}
