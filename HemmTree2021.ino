/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <WiFi.h>
#include <PubSubClient.h>

//this block for GITHUBf update
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

int stripLength=0;

// Replace the next variables with your SSID/Password combination
const char* ssid = "Gonzaga Guest"; //CenturyLink3314
const char* password = ""; //buet2kpjnnbtw9
String deviceMacAddress;

int dynamMode=0; //MQTT message can set this variable. loop() checks it each animation step to know what mode to be in

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

//GLOBAL VARAIBLES FOR DYNAMIC MODES
bool setupMode=false;
int prevMode=0;
int mode1firstPixelHue=0;
long mode3firstPixelHue=0;
int mode3a=0;

int mode2ctr=0;
int mode2r=0;
int mode2g=0;
int mode2b=0;

int mode3r=0;
int mode3g=0;
int mode3b=0;
int mode3directionR=1; //1 is up, -1 is down
int mode3directionG=1;
int mode3directionB=1;

bool mode4PulseOn=true;
int mode4ctr=0;

bool RGB; //logic in setup() for which strip type to use
bool GRB;
bool BRG;
bool acceptingInput=true;

bool justBooted=true;

// LED Pin
const int ledPin = 4;

//GITHUB update code. Change this number for each version increment
String FirmwareVer = {
  "0.133"
};
#define URL_fw_Version "https://raw.githubusercontent.com/NextGenTechBar/HemmTree/main/code_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/NextGenTechBar/HemmTree/main/ESP32_code.bin"

void firmwareUpdate();
int FirmwareVersionCheck();
void stripUpdate();

void setup() {
  Serial.begin(115200);
  
  pinMode(15,INPUT_PULLUP);
  pinMode(4,INPUT_PULLUP);
  if(digitalRead(4)==0){ //BRG if D4 is grounded (or D2 AND D15)
    BRG=true;
    RGB=false;
    GRB=false;
    Serial.println("BRG");
  }else if(digitalRead(15)==0){ //GRB if D15 is grounded (and D2 IS NOT)
    BRG=false;
    RGB=false;
    GRB=true;
    Serial.println("GRB");
  }else{ //RGB if no extra connections are made
    BRG=false;
    RGB=true;
    GRB=false;
    Serial.println("RGB");
  }
  
  
  randomSeed(analogRead(35));

  deviceMacAddress = WiFi.macAddress();
  Serial.println("BEGIN MAC: ");
  Serial.println(deviceMacAddress);

  //neopixel setup
    // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code


  //Read data from eeprom to set number of pixels
  int address = 0;
  int readId;
  EEPROM.begin(12); //EEPROM size

  /*
  //UNCOMMENT TO WRITE NEW STRING LENGTH TO EEPROM
  int boardId = 18;
  EEPROM.write(address, boardId);//EEPROM.put(address, boardId);
  address += sizeof(boardId); //update address value

  float param = 100; // CHANGE THIS VALUE TO WHAT YOU WANT THE STRING LENGTH TO BEEEEEEEEEEEEEEEEEEEEEEEE
  EEPROM.writeFloat(address, param);//EEPROM.put(address, param);
  EEPROM.commit();
  address = 0;
  //END TEMP ADDED 
  */
  
  readId = EEPROM.read(address); //EEPROM.get(address,readId);
  //Serial.print("Read Id = ");
  //Serial.println(readId);
  address += sizeof(readId); //update address value

  float readParam;
  EEPROM.get(address, readParam); //readParam=EEPROM.readFloat(address);
  //Serial.print("VALUE STORED IN EEPROM = ");
  //Serial.println(readParam);
  if(isnan(readParam)){
    stripLength=120; //default length
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
  //strip = Adafruit_NeoPixel(150, PIN, NEO_GRB + NEO_KHZ800);
  //stripLength=18; //temporary hard coded length
  strip.updateType(NEO_GRB + NEO_KHZ800);
  strip.setPin(PIN);
  strip.updateLength(stripLength);


  strip.begin();
  strip.setBrightness(255);
  strip.show(); // Initialize all pixels to 'off'
  //----


/*
  for(int i=0;i<stripLength;i++){
    strip.setPixelColor(i, strip.Color(255,0,0));
    strip.show();
  }
  for(int i=0;i<stripLength;i++){
    strip.setPixelColor(i, strip.Color(0,255,0));
    strip.show();
  }
  for(int i=0;i<stripLength;i++){
    strip.setPixelColor(i, strip.Color(0,0,255));
    strip.show();
  }
*/
  //all white on boot
  for(int i=0;i<stripLength;i++){
    strip.setPixelColor(i, strip.Color(255,255,255));
    if(stripLength==18){
      delay(25);
    }else{
     delay(5); 
    }
    strip.show();
  }

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

void stripUpdate(int pixel,int r,int g,int b){
  if(RGB){
    strip.setPixelColor(pixel, strip.Color(r,g,b));
  }else if(GRB){
    strip.setPixelColor(pixel, strip.Color(g,r,b));
  }else if(BRG){
    strip.setPixelColor(pixel, strip.Color(r,b,g));
  }
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
  int stringUpdate[stripLength][3];
  for(int i=0;i<stripLength;i++){
    for(int k=0;k<3;k++){
      stringUpdate[i][k]=0;
    }
  }

  if(messageTemp=="SLEEP"){
    acceptingInput=false;
  }
  if(messageTemp=="AWAKE"){
    acceptingInput=true;
  }
  if(acceptingInput){

    if(messageTemp=="FIRMWARE_UPDATE"){
    Serial.println("Received instruction to update firmware.");
    Serial.print("Active firmware version:");
    Serial.println(FirmwareVer);
    Serial.println("Will now check for new firmware..");
    if (FirmwareVersionCheck()) {
        firmwareUpdate();
      }
    }
  
    if(messageTemp.substring(0,5)=="PULSE"){ //send for example, PULSE9 to pulse strip 9 times. Intended use case is to pulse the hour
      int numPulses=messageTemp.substring(5).toInt();
      
      for(uint16_t i=0; i<stripLength; i++) {
        uint8_t LEDr =(strip.getPixelColor(i) >> 16);
        uint8_t LEDg =(strip.getPixelColor(i) >> 8);
        uint8_t LEDb =(strip.getPixelColor(i)) ;
        stringUpdate[i][0]=LEDr;
        stringUpdate[i][1]=LEDg;
        stringUpdate[i][2]=LEDb;
      }
  
      for(int chime=0;chime<numPulses;chime++){
      int speedFactorCourse=0; //higher is slower-->more delay (in millis)
      int speedFactorFine=0;   //in micros
        if(stripLength==18){ //slow down for shorter strips so longer ones can keep up
          speedFactorCourse=3;
          speedFactorFine=739;  //735 is SLIGHTLY too fast , 742 is too slow
        }
        for(int fadeOut=255;fadeOut>0;fadeOut--){
          if(fadeOut%5==0){
            for(int i=0;i<stripLength;i++){
              strip.setPixelColor(i, strip.Color(stringUpdate[i][0]*fadeOut/255.0,stringUpdate[i][1]*fadeOut/255.0,stringUpdate[i][2]*fadeOut/255.0)); //DO NOT use stripUpdate(), it will swap colors incorrectly
            }
            strip.show();
            delay(speedFactorCourse);
            delayMicroseconds(speedFactorFine);
          }
        }
  
        for(int fadeOn=0;fadeOn<255;fadeOn++){
          if(fadeOn%5==0){
            strip.setBrightness(fadeOn);
            for(int i=0;i<stripLength;i++){
              strip.setPixelColor(i, strip.Color(stringUpdate[i][0],stringUpdate[i][1],stringUpdate[i][2]));
            }
            strip.show();
            delay(speedFactorCourse);
            delayMicroseconds(speedFactorFine);
          }
        }
        delay(500);
      }
    }
  
    int repetitions=0;
    if(messageTemp.substring(0,5)=="COLOR"){ //repeats the received sequence for the whole string
      int msgLen=9;
      for(int i=0;i<stripLength;i++){
        stringUpdate[i][0]=messageTemp.substring(5+i*msgLen,8+i*msgLen).toInt(); //red
        stringUpdate[i][1]=messageTemp.substring(8+i*msgLen,11+i*msgLen).toInt(); //green
        stringUpdate[i][2]=messageTemp.substring(11+i*msgLen,14+i*msgLen).toInt(); //blue
        if(messageTemp[14+i*msgLen]==NULL){
          repetitions=i+1;
          break;
        }
      }
  
      //now fill the rest of the empty string with a repetition of the beginning
      for(int i=repetitions;i<stripLength;i++){
        stringUpdate[i][0]=stringUpdate[i%repetitions][0];
        stringUpdate[i][1]=stringUpdate[i%repetitions][1];
        stringUpdate[i][2]=stringUpdate[i%repetitions][2];
      }
  
      for(int i=0; i<stripLength; i++) {
        //strip.setPixelColor(i, strip.Color(stringUpdate[i][0],stringUpdate[i][1],stringUpdate[i][2]));
        stripUpdate(i,stringUpdate[i][0],stringUpdate[i][1],stringUpdate[i][2]);
        if(stripLength==18){
          delay(25);
        }else{
         delay(5); 
        }
        strip.show();
      }
  /*
    for(int i=0;i<stripLength;i++){
      for(int k=0;k<3;k++){
        Serial.print(stringUpdate[i][k]);
        Serial.print(" ");
      }
      Serial.println();
    }*/
    }
  
    int numColors=0;
    if(messageTemp.substring(0,5)=="FRACS"){ //divides the string in to equal quantities for each color
      int msgLen=9;
      for(int i=0;i<stripLength;i++){ //find out how many colors have been sent
        if(messageTemp[14+i*msgLen]==NULL){
          numColors=i+1;
          break;
        }
      }
  
      
      for(int k=0;k<numColors;k++){ //each block of colors
        for(int i=k*stripLength/numColors;i<(k+1)*stripLength/numColors;i++){
          int red=messageTemp.substring(5+k*msgLen,8+k*msgLen).toInt(); //red
          int green=messageTemp.substring(8+k*msgLen,11+k*msgLen).toInt(); //green
          int blue=messageTemp.substring(11+k*msgLen,14+k*msgLen).toInt(); //blue
          stripUpdate(i,red,green,blue);
          if(stripLength==18){
            delay(25);
          }else{
           delay(5); 
          }
          strip.show();
        }
      }
    }
  
    if(messageTemp.substring(0,5)=="OTHER"){ //modes that have parts generated/animated locally, but end static
      if(messageTemp.substring(5)=="random"){
        for(int i=0;i<stripLength;i++){
          stripUpdate(i,random(0,255),random(0,255),random(0,255));
          strip.show();
          if(stripLength==18){
            delay(25);
          }else{
           delay(5); 
          }
        }
      }

      if(messageTemp.substring(5)=="grinch"){
        for(int i=0;i<stripLength;i++){
          if(random(0,8)==0){
            stripUpdate(i,0,255,0);
            if(stripLength==18){
              delay(25);
            }else{
             delay(5); 
            }
            strip.show();
            stripUpdate(i+1,0,255,0);
            if(stripLength==18){
              delay(25);
            }else{
             delay(5); 
            }
            strip.show();
            stripUpdate(i+2,0,255,0);
            i=i+2;
          }else{
            stripUpdate(i,0,0,0);
          }
          if(stripLength==18){
            delay(25);
          }else{
           delay(5); 
          }
          strip.show();
        }
      }

      if(messageTemp.substring(5)=="differentcolors"){
        int red;
        int green;
        int blue;
        if(random(0,4)==0){ //25% of the time
          red = random(30,220);
        }else{
          if(random(0,2)==0){
            red = random(0,30);  
          }else{
            red = random(220,255);
          }
        }
        if(random(0,4)==0){ //25% of the time
          green = random(30,220);
        }else{
          if(random(0,2)==0){
            green = random(0,30);  
          }else{
            green = random(220,255);
          }
        }
        if(random(0,4)==0){ //25% of the time
          blue = random(30,220);
        }else{
          if(random(0,2)==0){
            blue = random(0,30);  
          }else{
            blue = random(220,255);
          }
        }
        for(int i=0;i<stripLength;i++){
          stripUpdate(i,red,green,blue);
          strip.show();
          if(stripLength==18){
            delay(25);
          }else{
           delay(5); 
          }
        }
      }

      if(messageTemp.substring(5)=="rainbow"){ //stationary rainbow. different from animation of rainbow
        for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
          int pixelHue = 0 + (i * 65536L / strip.numPixels());
          strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
            if(stripLength==18){
                delay(25);
            }else{
               delay(5); 
            }
            strip.show();
        }
      }

      if(messageTemp.substring(5)=="kaitlyn"){
        int red=0;
        int blue=0;
        int green=0;
        for(int x=0;x<3;x++){
          if(x==0){ //purple
            red=128;
            green=0;
            blue=128;
          }else if(x==1){ //blue
            red=0;
            green=0;
            blue=255;
          }else if(x==2){ //yellow
            red=255;
            green=255;
            blue=0;
          }
          for(int i=0;i<stripLength;i++){
            stripUpdate(i,red,green,blue);
            if(stripLength==18){delay(59);}else{delay(5);}
            strip.show();
          }
          for(int i=stripLength;i>=0;i--){
            stripUpdate(i,0,0,0);
            if(stripLength==18){delay(59);}else{delay(5);}
            strip.show();
          }
        }
        int numColors=3;
        int msgLen=9;
        messageTemp="-----255255000255000255000255255000000255";
        for(int k=0;k<numColors;k++){ //each block of colors
          for(int i=k*stripLength/numColors;i<(k+1)*stripLength/numColors;i++){
            int red=messageTemp.substring(5+k*msgLen,8+k*msgLen).toInt(); //red
            int green=messageTemp.substring(8+k*msgLen,11+k*msgLen).toInt(); //green
            int blue=messageTemp.substring(11+k*msgLen,14+k*msgLen).toInt(); //blue
            stripUpdate(i,red,green,blue);
            if(stripLength==18){
              delay(59);
            }else{
             delay(5); 
            }
            strip.show();
          }
        }
      }
  
      if(messageTemp.substring(5)=="fred"){
        int numColors=3;
        int msgLen=9;
        messageTemp="-----000000255255000000255255000";
        for(int k=0;k<numColors;k++){ //each block of colors
          for(int i=k*stripLength/numColors;i<(k+1)*stripLength/numColors;i++){
            int red=messageTemp.substring(5+k*msgLen,8+k*msgLen).toInt(); //red
            int green=messageTemp.substring(8+k*msgLen,11+k*msgLen).toInt(); //green
            int blue=messageTemp.substring(11+k*msgLen,14+k*msgLen).toInt(); //blue
            stripUpdate(i,red,green,blue);
            if(stripLength==18){
              delay(59);
            }else{
             delay(5); 
            }
            strip.show();
          }
        }

        int tempStorage[stripLength][3];
        for(uint16_t i=0; i<stripLength; i++) {
          uint8_t LEDr =(strip.getPixelColor(i) >> 16);
          uint8_t LEDg =(strip.getPixelColor(i) >> 8);
          uint8_t LEDb =(strip.getPixelColor(i)) ;
          tempStorage[i][0]=LEDr;
          tempStorage[i][1]=LEDg;
          tempStorage[i][2]=LEDb;
        }
        delay(1000);
        strip.clear();
        strip.show();

        delay(1000);
        int charDelay=30;
        int letterDelay=300;
        int dashDelay=700;
        int dotDelay=400;

        //t
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dashDelay);

        strip.clear();
        strip.show();
        delay(letterDelay);

        //b
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dashDelay);
        strip.clear();
        strip.show();
        delay(charDelay);
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dotDelay);
        strip.clear();
        strip.show();
        delay(charDelay);
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dotDelay);
        strip.clear();
        strip.show();
        delay(charDelay);
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dotDelay);
        
        strip.clear();
        strip.show();
        delay(letterDelay);

        //i
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dotDelay);
        strip.clear();
        strip.show();
        delay(charDelay);
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dotDelay);

        strip.clear();
        strip.show();
        delay(letterDelay);

        //y
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dashDelay);
        strip.clear();
        strip.show();
        delay(charDelay);
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dotDelay);
        strip.clear();
        strip.show();
        delay(charDelay);
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dashDelay);
        strip.clear();
        strip.show();
        delay(charDelay);
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dashDelay);
        
        strip.clear();
        strip.show();
        delay(letterDelay);

        //t
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dashDelay);

        strip.clear();
        strip.show();
        delay(letterDelay);

        //c
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dashDelay);
        strip.clear();
        strip.show();
        delay(charDelay);
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dotDelay);
        strip.clear();
        strip.show();
        delay(charDelay);
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dashDelay);
        strip.clear();
        strip.show();
        delay(charDelay);
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
        delay(dotDelay);
        strip.clear();
        strip.show();
        delay(2000);
        for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
        strip.show();
      }
      }
    
  
    if(messageTemp.substring(0,5)=="SHORT"){ //for modes that do an animation, then return to previous (so they don't interrupt dynam
      int tempStorage[stripLength][3];
      for(uint16_t i=0; i<stripLength; i++) {
        uint8_t LEDr =(strip.getPixelColor(i) >> 16);
        uint8_t LEDg =(strip.getPixelColor(i) >> 8);
        uint8_t LEDb =(strip.getPixelColor(i)) ;
        tempStorage[i][0]=LEDr;
        tempStorage[i][1]=LEDg;
        tempStorage[i][2]=LEDb;
      }

      if(messageTemp.substring(5)=="invalid"){
        for(int i=0;i<2;i++){
          for(int i=255;i>0;i--){
            if(i%10==0){
              strip.setBrightness(i);
              strip.show();
              if(stripLength==18){
                delay(10);
              }else{
               delay(7); 
              }
            }
          }
          for(int i=0;i<255;i++){
            if(i%10==0){
              for(int i=0;i<stripLength;i++){
                stripUpdate(i,255,0,0);
              }
              strip.setBrightness(i);
              strip.show();
              if(stripLength==18){
                delay(10);
              }else{
               delay(7); 
              }
            }
          }
        }
      }
      
      if(messageTemp.substring(5)=="thayne"){
        for(int i=0;i<20;i++){
          int col=random(0,3);
          if(col==0){
            for(int i=0;i<stripLength;i++){
              stripUpdate(i,255,0,0);
            }
            strip.show();
          }else if(col==1){
            for(int i=0;i<stripLength;i++){
              stripUpdate(i,0,0,255);
            }
            strip.show();
          }else{
            for(int i=0;i<stripLength;i++){
              stripUpdate(i,255,255,255);
            }
            strip.show();
          }
          delay(120);
        }
      }

      if(messageTemp.substring(5)=="jordan"){
        for(int k=0;k<20;k++){
          for(int i=0;i<stripLength;i++){
            stripUpdate(i,random(0,255),random(0,255),random(0,255));
          }
          delay(120);
          strip.show();
        }
      }
  
      //return strip to previous state (fade out then in)
      for(int i=255;i>0;i--){
        if(i%5==0){
          strip.setBrightness(i);
          strip.show();
          if(stripLength==18){
            if(messageTemp.substring(5)=="invalid"){
              delay(10);
            }else{
              delay(25);
            }
          }else{
           if(messageTemp.substring(5)=="invalid"){
            delay(7); 
           }else{
            delay(5); 
           }
        }
        }
      }
      for(int i=0;i<255;i++){
        if(i%5==0){
          for(int i=0;i<stripLength;i++){ strip.setPixelColor(i, strip.Color(tempStorage[i][0],tempStorage[i][1],tempStorage[i][2]));} //DO NOT use stripUpdate()--it will incorrectly swap colors since we are using getpixelcolor
          strip.setBrightness(i);
          strip.show();
          if(stripLength==18){
            delay(25);
          }else{
           delay(5); 
        }
        }
      }
    }
  
    if(messageTemp.substring(0,5)=="DYNAM"){ //modes that involve motion
      if(messageTemp.substring(5)=="rainbow"){
        dynamMode=1;
      }else if(messageTemp.substring(5)=="colorwipe"){
        dynamMode=2;
      }else if(messageTemp.substring(5)=="chase"){
        dynamMode=3;    
      }else if(messageTemp.substring(5)=="fade"){
        dynamMode=4;
      }else if(messageTemp.substring(5)=="pulses"){
        dynamMode=5;
      }
    }else if (messageTemp.substring(0,5)!="PULSE" && messageTemp.substring(0,5)!="SHORT"){ //reset to inactive animation if any message prefix other than DYNAM or PULSE comes through
      dynamMode=0; 
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    Serial.print("mac address1: ");
    Serial.println(deviceMacAddress);
    if (client.connect(deviceMacAddress.c_str())) { //make client ID the mac address to ensure it's unique
      Serial.println("connected");
      Serial.print("mac address2: ");
      Serial.println(deviceMacAddress);
      // Subscribe
      client.subscribe("GUHemmTree");
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
    if(justBooted && client.connected()){
      client.publish("GUHemmTreeBootLog",deviceMacAddress.c_str()); //only do this the first time
      justBooted=false;
    }else if(!justBooted && client.connected()){
      client.publish("GUHemmTreeReconnectLog",deviceMacAddress.c_str()); //do this any time we were reconnecting and re-established connection
    }
  }
  client.loop(); //checks for new MQTT msg

  if(dynamMode!=0){
    setupMode=false;
    if(prevMode!=dynamMode){
      setupMode=true;
      prevMode=dynamMode;
    }
  }else{
    prevMode=0;
  }
  
  if(dynamMode==1){
    rainbow();
  }else if(dynamMode==2){
    colorWipe();
  }else if(dynamMode==3){
    chase();
  }else if(dynamMode==4){
    fade();
  }else if(dynamMode==5){
    pulses();
  }
  

}

//DYNAMIC ANIMATION FUNCTIONS
void rainbow(){
  
  if(setupMode){ //first loop of this function, set it up
    mode1firstPixelHue=0;
  }
  
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    int pixelHue = mode1firstPixelHue + (i * 65536L / strip.numPixels());
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    if(setupMode){ //if it's the first time, incrementally set strip to rainbow colors as a transition
      if(stripLength==18){
          delay(25);
      }else{
         delay(5); 
      }
      strip.show();
    }
  }
  strip.show(); // Update strip with new contents
  delay(11);  // Pause for a moment
  
  mode1firstPixelHue+=256; //emulating for loop
  if(mode1firstPixelHue>=5*65536){ //emulating for loop
    mode1firstPixelHue=0;
  }

}

void colorWipe(){
  if(setupMode){
    mode2r = random(0,255);
    mode2g = random(0,255);
    mode2b = random(0,255);
    mode2ctr=0;
  }

  stripUpdate(mode2ctr,mode2r,mode2g,mode2b);
  strip.show();
  if(stripLength==18){
    delay(100);  
  }else{
    delay(30);
  }
  
  mode2ctr++;
  if(mode2ctr>=stripLength){ //reset and make new colors
    delay(500);
    mode2ctr=0;
    if(random(0,4)==0){ //25% of the time
      mode2r = random(30,220);
    }else{
      if(random(0,2)==0){
        mode2r = random(0,30);  
      }else{
        mode2r = random(220,255);
      }
    }
    if(random(0,4)==0){ //25% of the time
      mode2g = random(30,220);
    }else{
      if(random(0,2)==0){
        mode2g = random(0,30);  
      }else{
        mode2g = random(220,255);
      }
    }
    if(random(0,4)==0){ //25% of the time
      mode2b = random(30,220);
    }else{
      if(random(0,2)==0){
        mode2b = random(0,30);  
      }else{
        mode2b = random(220,255);
      }
    }
  }
}

void chase(){
  if(setupMode){
    mode3firstPixelHue = 0;
    mode3a = 0;
  }
  int wait=60;

  for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
    strip.clear();         //   Set all pixels in RAM to 0 (off)
    // 'c' counts up from 'b' to end of strip in increments of 3...
    for(int c=b; c<strip.numPixels(); c += 3) {
      // hue of pixel 'c' is offset by an amount to make one full
      // revolution of the color wheel (range 65536) along the length
      // of the strip (strip.numPixels() steps):
      int      hue   = mode3firstPixelHue + c * 65536L / strip.numPixels();
      uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
      strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
    }
    strip.show();                // Update strip with new contents
    delay(wait);                 // Pause for a moment
    mode3firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
  }
  mode3a++;
  if(mode3a>29){
    mode3a=0;
  }
  
}

void fade(){
  if(setupMode){
    mode3r = random(0,255);
    mode3g = random(0,255);
    mode3b = random(0,255);
  }

  if(random(0,20)==0){ //20% of the time, change the direction
    mode3directionR*=-1;
  }
  if(random(0,20)==0){
    mode3directionG*=-1;
  }
  if(random(0,20)==0){
    mode3directionG*=-1;
  }
  
  int changeRed=random(0,5);
  int changeGreen=random(0,5);
  int changeBlue=random(0,5);

  mode3r = mode3r+changeRed*mode3directionR;
  mode3g = mode3g+changeGreen*mode3directionG;
  mode3b = mode3b+changeBlue*mode3directionB;
  
  mode3r = constrain(mode3r,0,255); //keep within range if it goes outside
  mode3g = constrain(mode3g,0,255);
  mode3b = constrain(mode3b,0,255);
  
  for(int i=0;i<stripLength;i++){
    stripUpdate(i,mode3r,mode3g,mode3b);
  }
  strip.show();
  delay(20);
}

void pulses(){
  if(setupMode){
    mode4ctr=255;
    mode4PulseOn=false; //fade off first
    mode3r = constrain(random(0,255),0,255); //keep within range if it goes outside
    mode3g = constrain(random(0,255),0,255);
    mode3b = constrain(random(0,255),0,255);

    //for(int i=0;i<stripLength;i++){
      //strip.setPixelColor(i,strip.Color(mode3r,mode3g,mode3b));
      
    //}
    
    //In the future: fade off
  }

  
  int speedFactorCourse=random(48,52);
  int speedFactorFine=0;
  if(stripLength==18){ //slow down for shorter strips so longer ones can keep up
    speedFactorCourse=random(48,52);
    speedFactorFine=739;  
  }


  if (mode4PulseOn){ //if we're fading on not off
    
    if(mode4ctr<255){
      if(mode4ctr%5==0){
        strip.setBrightness(mode4ctr);
        for(int i=0;i<stripLength;i++){
          strip.setPixelColor(i, strip.Color(mode3r*mode4ctr/255.0,mode3g*mode4ctr/255.0,mode3b*mode4ctr/255.0));
        }
        strip.show();
        delay(speedFactorCourse);
        delayMicroseconds(speedFactorFine);
      }
      mode4ctr++;
    }else{
      mode4ctr=255;
      mode4PulseOn=false;
    }
    
  }else{ //if we're fading off not on
    if(mode4ctr>-1){
      if(mode4ctr%5==0){
        for(int i=0;i<stripLength;i++){
          strip.setPixelColor(i, strip.Color(mode3r*mode4ctr/255.0,mode3g*mode4ctr/255.0,mode3b*mode4ctr/255.0));
        }
        strip.show();
        delay(speedFactorCourse);
        delayMicroseconds(speedFactorFine);
      }
      mode4ctr--;
    }else{
      mode4PulseOn=true;
      mode4ctr=0;
      mode3r = constrain(random(0,255),0,255); //keep within range if it goes outside
      mode3g = constrain(random(0,255),0,255);
      mode3b = constrain(random(0,255),0,255);
    }
  }
  
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
      strip.clear();
      for(int i=0;i<stripLength;i++){
        if(i%5==0){
          strip.setPixelColor(i, strip.Color(50,50,50));
        }
      }
      strip.show();
      return 1;
    }
  } 
  return 0;  
}
