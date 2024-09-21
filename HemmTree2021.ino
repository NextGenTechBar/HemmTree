//MAC list moved to seperate file in GitHub -- https://github.com/NextGenTechBar/HemmTree/blob/main/HemmTreeMacAddresses.csv
#include <WiFi.h>
#include <PubSubClient.h>

//to mark new code as valid and prevent rollback. See  esp_ota_mark_app_valid_cancel_rollback() in code
#include <esp_ota_ops.h>

//this block for GITHUB update
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
//#include "cert.h" //no longer necessary with setInsecure()
#include <WiFiManager.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 33
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(150, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(); //length, etc is read from eeprom and set in setup()

int stripLength=0;

// Replace the next variables with your SSID/Password combination
//const char* ssid = "StartideRising"; //CenturyLink3314
//const char* password = "3arthClan2book"; //buet2kpjnnbtw9
//const char* ssid = "Blaine";
//const char* password = "cowsrock";

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

int mode2ctr=0; //these also used in mode7
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
int mode4SpeedFactor;

int twinkleBeginCtr=0; //used to fade in to twinkle mode slower at the beginning

uint8_t* stripCopyRed; //an array to store a copy of the strip values in. Useful for modes like twinkle() when each LED's next state is based on it's own previous state. This size will be set to stripLength at runtime
uint8_t* stripCopyGreen;
uint8_t* stripCopyBlue;

uint8_t* stripSecondCopyRed; //an array to store a copy of the strip values in. Used in new twinkle modifier to store the colors in the strip from before the mode is activated.
uint8_t* stripSecondCopyGreen;
uint8_t* stripSecondCopyBlue;


bool RGB; //logic in setup() for which strip type to use
bool GRB;
bool BRG;
bool acceptingInput=true;

bool justBooted=true;
bool inCaptivePortal=false;
bool brightnessPotConnected=false;
int lastBrightnessValue=255;

bool isMiniTree=false; //setup will read pin 13 and set this true if the pin is connected, indicating it is a mini-tree

// LED Pin
const int ledPin = 4;

//GITHUB update code. Change this number for each version increment
String FirmwareVer = {
  "0.174"
};
#define URL_fw_Version "https://raw.githubusercontent.com/NextGenTechBar/HemmTree/main/code_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/NextGenTechBar/HemmTree/main/ESP32_code.bin"

void firmwareUpdate();
int FirmwareVersionCheck();
void stripUpdate();

void setup() {
  //Prevent rollback to previous firmware
  //WEIRD NOTE: automatic rollback only occors automatically sometimes. Maybe dependant on the last computer that uploaded serial code to it?
  esp_ota_mark_app_valid_cancel_rollback();
  
  Serial.begin(115200);
  Serial.println("----------");
  pinMode(15,INPUT_PULLUP);
  pinMode(4,INPUT_PULLUP);
  pinMode(14,INPUT_PULLUP);
  delay(10); //give voltage levels time to stabalize before reading config pins
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

  deviceMacAddress = WiFi.macAddress();
  Serial.println("BEGIN MAC: ");
  Serial.println(deviceMacAddress);

  pinMode(13,INPUT_PULLUP);
  delay(10); //give voltage levels time to stabalize before reading config pins
  if(!digitalRead(13)){
    isMiniTree=true; 
  }
  //This next line is because I handed out several mini trees before realizing the wrong pin was grounded (it was 35 instead of 13). This line ensures those few trees behave as mini trees anyway
  //order: Mr. Castaneda, G&G, Chelsey, Kenzie
  if(deviceMacAddress=="D4:D4:DA:46:EB:38" || deviceMacAddress=="D4:D4:DA:59:28:08" || deviceMacAddress=="D4:D4:DA:53:63:2C" || deviceMacAddress=="D4:D4:DA:59:27:A4"){
    isMiniTree=true; 
  }
  
  
  randomSeed(analogRead(35));

  

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
  Serial.println("NEW: strip length update setting also availible to user in wifi setup page. Boot ESP while out of range of saved network to access");

  EEPROM.end();
  //strip = Adafruit_NeoPixel(150, PIN, NEO_GRB + NEO_KHZ800);
  //stripLength=18; //temporary hard coded length
  strip.updateType(NEO_GRB + NEO_KHZ800);
  strip.setPin(PIN);
  strip.updateLength(stripLength);
  stripCopyRed=(uint8_t*)calloc(stripLength,sizeof(uint8_t)); //set size of stripCopy to be equal to strip length
  stripCopyGreen=(uint8_t*)calloc(stripLength,sizeof(uint8_t));
  stripCopyBlue=(uint8_t*)calloc(stripLength,sizeof(uint8_t));

  stripSecondCopyRed=(uint8_t*)calloc(stripLength,sizeof(uint8_t)); //set size of stripSecondCopy to be equal to strip length
  stripSecondCopyGreen=(uint8_t*)calloc(stripLength,sizeof(uint8_t));
  stripSecondCopyBlue=(uint8_t*)calloc(stripLength,sizeof(uint8_t));

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

/*
//TESTING
for(int i=0;i<255;i++){
  for(int k=0;k<stripLength;k++){
    strip.setPixelColor(k, strip.Color(i,i,i));
  }
  if(i%10==0){
    for(int k=0;k<stripLength;k++){
      strip.setPixelColor(k, strip.Color(100,0,0));
    }
    delay(300);
  }
  strip.show();
  delay(50);
}
*/
  //all white on boot
  for(int i=0;i<stripLength;i++){
    if(isMiniTree){ //set mini trees to less bright on boot, otherwise max brightness white will overwhelm potential 1A power supply, not giving them a chance to boot fully and enable lower-brightness mode (if set by pin)
      stripUpdate(i,40,40,40);
    }else{
      stripUpdate(i,255,255,255);
    }
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
/*
  //TESTING MINI TREE
  client.connect(deviceMacAddress.c_str());
  Serial.println("BEGIN TESTTTTT");
  client.publish("somethinglikethisok","test beginning");
  for(int i=100;i<255;i++){
    char temp[]="100";
    itoa(i,temp,10);
    client.publish("blaineDebugging",temp);
    
    Serial.println(i);
    strip.setBrightness(i);
    strip.show();
    delay(500);
  }
  client.subscribe("GUHemmTree");  //RESULTS: 130 brightness breaks 1A charger, ext cable breaks early
  //END TEST
*/

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
    stripCopyRed[pixel]=r;
    stripCopyGreen[pixel]=g;
    stripCopyBlue[pixel]=b;
  }else if(GRB){
    strip.setPixelColor(pixel, strip.Color(g,r,b));
    stripCopyRed[pixel]=g;
    stripCopyGreen[pixel]=r;
    stripCopyBlue[pixel]=b;
  }else if(BRG){
    strip.setPixelColor(pixel, strip.Color(r,b,g));
    stripCopyRed[pixel]=r;
    stripCopyGreen[pixel]=b;
    stripCopyBlue[pixel]=g;
  }

  
}

void setup_wifi() {
  WiFiManager manager;
  manager.setDebugOutput(false);
  //manager.resetSettings();
  
  WiFiManagerParameter stripLengthParameter("parameterId", "Strip Length", String(stripLength).c_str(), 5);
  manager.addParameter(&stripLengthParameter);
  //would be a useful way to determine strip order, but I already have code to set this by connecting certain pins on the ESP, so maybe we'll switch to this in the future, but for now pin woring works.
  //WiFiManagerParameter parameterTwo("parameterId2", "Strip Color Order", "RGB", 3);
  //manager.addParameter(&parameterTwo);

  Serial.println("Attempting to connect to saved network");
  WiFi.begin();
  unsigned long startTime=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-startTime<14000){
    Serial.print(".");
    delay(500);
    if(WiFi.status()==WL_NO_SSID_AVAIL || WiFi.status()==WL_CONNECT_FAILED){
      break;
    }
  }
  Serial.println();

  if(WiFi.status()!=WL_CONNECTED){ //WIFI MANAGER BLOCK
    String networkName="HemmTree ESP-"+WiFi.macAddress();
    Serial.print("Connection to saved network failed, starting config AP on: ");
    Serial.println(networkName);

    //set lights to indicate that we are in config mode
    //set lights to indicate that we are in config mode
    for(int i=0;i<stripLength;i++){
      if(i%5==0){
        stripUpdate(i,0,0,50);
      }else{
        strip.setPixelColor(i, strip.Color(0,0,0));
      }
      strip.show();
      delay(10);
    }

    manager.setTimeout(60*5); //if no pages are loaded on the setup AP within this many seconds, reboot in an attempt to connect to the saved network again.
    if(!manager.autoConnect(networkName.c_str(),"")){
      Serial.println("WifiManager portal timeout. Resetting now to attempt connection again. Will launch AP again on reboot if connection fails again");
      Serial.println("\n\n");
      ESP.restart();
    }
    Serial.print("Successfully connected to ");
    Serial.println(WiFi.SSID());

    int stripLengthInputAsInt= atoi(stripLengthParameter.getValue());
    if(stripLengthInputAsInt==0){
      Serial.print("INVALID STRIP LENGTH (user did not enter number)\nUSING DEFAULT (or previous) LENGTH: ");
      Serial.println(stripLength);
    }else if(stripLengthInputAsInt!=stripLength){ //only bother updating if they actually changed the length
      Serial.print("Strip Length: ");
      Serial.println(stripLength);
      stripLength=stripLengthInputAsInt;
      Serial.println("------------------DEBUG ONE");
      strip.clear(); //clear entire strip before potentially shortening length, so excess lights are off
      strip.show();
      strip.updateLength(stripLength);
      stripCopyRed=(uint8_t*)calloc(stripLength,sizeof(uint8_t)); //set size of stripCopy to be equal to strip length
      stripCopyGreen=(uint8_t*)calloc(stripLength,sizeof(uint8_t));
      stripCopyBlue=(uint8_t*)calloc(stripLength,sizeof(uint8_t));

      stripSecondCopyRed=(uint8_t*)calloc(stripLength,sizeof(uint8_t)); //set size of stripSecondCopy to be equal to strip length
      stripSecondCopyGreen=(uint8_t*)calloc(stripLength,sizeof(uint8_t));
      stripSecondCopyBlue=(uint8_t*)calloc(stripLength,sizeof(uint8_t));
      
      //WRITE NEW STRIPLENGTH TO EEPROM
      int boardId = 18;
      int address=0;
      int readId;
      EEPROM.begin(12);
      EEPROM.write(address, boardId);//EEPROM.put(address, boardId);
      address += sizeof(boardId); //update address value  (I think this is just getting ready to put in another value but we don't have one so it doesn't matter)
    
      float param = stripLength; 
      EEPROM.writeFloat(address, param);//EEPROM.put(address, param);
      EEPROM.commit();
      EEPROM.end();
    }
    //light up white now that we're connected and ready to go! (if we implement persistant MQTT messages, this may be immediately overridden by the last mode. Maybe a case for turning it black backwards instead of white forwards)
    for(int i=0;i<strip.numPixels();i++){
      strip.setPixelColor(i, strip.Color(50,50,50)); //this is usually not seen, but on cases where it is (usually turns blue, but then connects last minute), it shouldn't overload mini-trees
      strip.show();
      delay(15);
    }
  }

  Serial.print("Succesfully connected to ");
  Serial.println(WiFi.SSID());

  //OLD CODE, PRIOR TO WIFI MANAGER (just using hard-coded credentials)
  /*
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
  */
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

    if(messageTemp=="WHOSTHERE"){
      client.publish("GUHemmTree/connectionLog",("PING,"+deviceMacAddress).c_str());
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
          uint32_t tempPixelHue = strip.gamma32(strip.ColorHSV(pixelHue));
          //strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
          stripUpdateHSV(i,tempPixelHue); //update stripCopy even though we're not using the stripUpdate() function
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
        if(messageTemp.substring(5)=="emmag"){ //for Emma Gashi
          int solidCount=stripLength/6; //the number of LEDs at the very begining and end that are solid red or white
          for(int i=0;i<solidCount;i++){
            stripUpdate(i,255,0,0);
            if(stripLength==18){
              delay(25);
            }else{
              delay(5); 
            }
            strip.show();
          }
          for(int i=solidCount;i<stripLength/2;i++){
            stripUpdate(i,255,0,255*((float(i-solidCount))/(stripLength/2-solidCount)));
            if(stripLength==18){
              delay(25);
            }else{
              delay(5); 
            }
            strip.show();
          }
          for(int i=stripLength/2;i<stripLength;i++){ 
            stripUpdate(i,255,(255*(float(i-stripLength/2)/(stripLength/2))),255);
            if(stripLength==18){
              delay(25);
            }else{
              delay(5); 
            }
            strip.show();
          }
        }
        if(messageTemp.substring(5)=="andrewBlue"){ //for Andrew Culver from USBank who always comes in to change our lights to Blue ❤
          bool validPixel=false; //change to true once we find a pixel to change that isn't already blue.
          int randomLight=random(0,stripLength-1);
          Serial.println(randomLight);
          for(int i=0;i<stripLength*3;i++){ //dirty check. But if we've tried 3x the number of lights, it's probably safe to say we've done all the lights already...
            Serial.println("RGB: ");
            Serial.print(stripCopyRed[randomLight]);
            Serial.print(",");
            Serial.print(stripCopyGreen[randomLight]);
            Serial.print(",");
            Serial.println(stripCopyBlue[randomLight]);
            if(!(stripCopyRed[randomLight]==0 && stripCopyGreen[randomLight]==0 && stripCopyBlue[randomLight]==180)){
              validPixel=true;
              stripUpdate(randomLight,0,0,180);
              strip.show();
              Serial.println("updated!");
            }else{
              randomLight=random(0,stripLength-1);
              Serial.print("loop ");
              Serial.println(randomLight);
            }
            if(validPixel){
              break;
            }
          }
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
        int animSteps=10;
        if(lastBrightnessValue<200){ //if the strip is dimmer, animate slower cause it'll go faster with fewer brightness levels to iterate through
          animSteps=3;
        }
        for(int i=0;i<2;i++){
          for(int i=lastBrightnessValue;i>0;i--){ //changed 255 to lastBrightnessValue to prevent mini trees supplied by 1A supply from getting overloaded by suddenly being set to max
            if(i%animSteps==0){
              strip.setBrightness(i);
              strip.show();
              if(stripLength==18){
                delay(10);
              }else{
               delay(7); 
              }
            }
          }
          for(int i=0;i<lastBrightnessValue;i++){ //changed 255 to lastBrightnessValue to prevent mini trees supplied by 1A supply from getting overloaded by suddenly being set to max
            if(i%animSteps==0){
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

      if(messageTemp.substring(5)=="emma"){
        for(int i=0;i<40;i++){
          int col=random(0,2);
          if(col==0){
            for(int i=0;i<stripLength;i++){
              stripUpdate(i,255,0,190);
            }
            strip.show();
          }else{
            for(int i=0;i<stripLength;i++){
              stripUpdate(i,255,255,255);
            }
            strip.show();
          }
          delay(90);
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
      for(int i=lastBrightnessValue;i>0;i--){
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
      for(int i=0;i<lastBrightnessValue;i++){
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
      }else if(messageTemp.substring(5)=="twinkle"){
        dynamMode=6;
      }else if(messageTemp.substring(5)=="multicolorwipe"){
        dynamMode=7;
      }else if(messageTemp.substring(5)=="xmaschase"){
        dynamMode = 8;
      }else if(messageTemp.substring(5)=="twinkleMod"){
        dynamMode = 9;
      }
    }else if (messageTemp.substring(0,5)!="PULSE" && messageTemp.substring(0,5)!="SHORT"){ //reset to inactive animation if any message prefix other than DYNAM or PULSE comes through
      dynamMode=0; 
    }
  }
}

void reconnect() {
  int retryCtr=0;
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    Serial.print("mac address1: ");
    Serial.println(deviceMacAddress);
    if (client.connect(deviceMacAddress.c_str(),"","","GUHemmTree/connectionLog",1,false,("DISCONNECT,"+deviceMacAddress).c_str())) { //make client ID the mac address to ensure it's unique
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

      retryCtr++;
      if(retryCtr>5){ //if disconnected from wifi for ~50 seconds (including connect attempt time), restart the ESP to reconnect. Otherwise sometimes it gets stuck in this loop due to some sort of ESP firmware bug
        ESP.restart();
      }
    }   
  }
}
void loop() {
  if (!client.connected()) {
    reconnect();
    if(justBooted && client.connected()){
      client.publish("GUHemmTree/connectionLog",("BOOT,"+deviceMacAddress).c_str()); //only do this the first time
      justBooted=false;
    }else if(!justBooted && client.connected()){
      client.publish("GUHemmTree/connectionLog",("RECONNECT,"+deviceMacAddress).c_str()); //do this any time we were reconnecting and re-established connection
    }
  }else if(inCaptivePortal){ //if we were in a captive portal (showing orange), but got here that means we aren't anymore so we should update the strip to stop being orange
    inCaptivePortal=false;
    for(int i=0;i<strip.numPixels();i++){
      strip.setPixelColor(i, strip.Color(40,40,40));
      strip.show();
      delay(15);
    }
  }
  client.loop(); //checks for new MQTT msg



  //do this every loop because on mini-tree, users may change jumper while running
  if(digitalRead(14)){ //if a wire is soldered from here to ground, that means a potentiometer is also connected to D35, and those readings should be used for brightness. Otherwise, set max.
    brightnessPotConnected=false;
    if(lastBrightnessValue<255){ //do this if someone removed the D14 jumper while running (meaning they want it to go to max brightness)
      strip.setBrightness(255);
      lastBrightnessValue=255;
    }
  }else{
    brightnessPotConnected=true;
  }

  if(brightnessPotConnected){
    int newBrightness=map(analogRead(34), 0, 4095, 0, 255);
    if(lastBrightnessValue>newBrightness+10 || lastBrightnessValue<newBrightness-10){
      strip.setBrightness(newBrightness);
      lastBrightnessValue=newBrightness; //lol, I forgot to do this for over a year, so all this fancy smoothing code wasn't actually doing anything LOL
      //strip.show(); //DON'T DO THIS: it causes colors with components that are not maximum to fade out. Leaving it out, brightness is only updated whenever strip.show() is called after setting the strip to another color, so color fidelity is preserved
    }
  }

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
  }else if(dynamMode==6){
    twinkle();
  }else if(dynamMode==7){
    multiColorWipe();
  }else if(dynamMode == 8){
    xmasChase();
  }else if(dynamMode == 9){
    twinkleMod();
  }
  

}

//DYNAMIC ANIMATION FUNCTIONS
void rainbow(){
  
  if(setupMode){ //first loop of this function, set it up
    mode1firstPixelHue=0;
  }
  
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    int pixelHue = mode1firstPixelHue + (i * 65536L / strip.numPixels());
    //strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    uint32_t tempPixelHue = strip.gamma32(strip.ColorHSV(pixelHue));
    stripUpdateHSV(i, tempPixelHue); //store current color in stripCopy even though we're not using stripUpdate()
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
      //strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      stripUpdateHSV(c,color); //update stripCopy even though we're not using stripUpdate()
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
//----------------------------------------------------------------------------------------------------------XMAS CHASE---------------------------------------------------------------------------------------
void xmasChase(){
  // Serial.println("in xamas chase");
  uint32_t color = strip.Color(255,   0,   0);
  for(int b = 0; b < 3; b++) { //  'b' counts from 0 to 2...
    strip.clear();         //   Set all pixels in RAM to 0 (off)
    // 'c' counts up from 'b' to end of strip in increments of 3...
    for(int c = b; c < strip.numPixels(); c += 3) {
      strip.setPixelColor(c,color);
    }
    color = strip.Color(0,   170,   0);
    strip.setPixelColor(b, color);
    strip.show();                // Update strip with new contents
    delay(100);                 // Pause for a moment
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

    mode4SpeedFactor=random(48,52);

    for(int i=0; i<stripLength; i++) {
      //strip.setPixelColor(i, strip.Color(stringUpdate[i][0],stringUpdate[i][1],stringUpdate[i][2]));
      stripUpdate(i,mode3r,mode3g,mode3b);
      if(stripLength==18){
        delay(25);
      }else{
       delay(5); 
      }
      strip.show();
    }

    delay(random(0,4000)); //initialize ornaments at different times so they're out of phase
  }

  
  int speedFactorCourse=mode4SpeedFactor;
  int speedFactorFine=739;


  if (mode4PulseOn){ //if we're fading on not off
    
    if(mode4ctr<255){
      if(mode4ctr%5==0){
        //strip.setBrightness(mode4ctr); //don't do this. It gets ornaments stuck at this brightness level
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

void twinkle(){
  if(setupMode){ //If we ever update stripCopy EVERY time we update the strip, we should change this to start at the previous mode and twinkle out from there instead of filling with random
    twinkleBeginCtr=0;
  }

  for(int i=0;i<stripLength;i++){
    //uint8_t ra =(strip.getPixelColor(i) >> 16); //DON'T USE THIS FUNCTION, REFERENCE GLOBAL COPY INSTEAD
    //uint8_t ga =(strip.getPixelColor(i) >> 8);
    //uint8_t ba =(strip.getPixelColor(i));

    int r=stripCopyRed[i];
    int g=stripCopyGreen[i];
    int b=stripCopyBlue[i];
    
    if(r%2==0){ //if even, go up
      r+=2;
    }else{ //if odd, go down
      r-=2;
    }
    if(g%2==0){ //if even, go up
      g+=2;
    }else{ //if odd, go down
      g-=2;
    }
    if(b%2==0){ //if even, go up
      b+=2;
    }else{ //if odd, go down
      b-=2;
    }

    int dirSwitch=20;
    if(random(0,dirSwitch)==0){ //certain percentage of the time, add 1 which switches the direction
      r++;
    }
    if(random(0,dirSwitch)==0){
      g++;
    }
    if(random(0,dirSwitch)==0){
      b++;
    }

    r = constrain(r,2,253); //keep within range if it goes outside (NOT 0 to 255)
    g = constrain(g,2,253);
    b = constrain(b,2,253);

    strip.setPixelColor(i,strip.Color(r,g,b));
    stripCopyRed[i]=r;
    stripCopyGreen[i]=g;
    stripCopyBlue[i]=b;
  }
  strip.show();
  if(twinkleBeginCtr<50){ //make the initial transition slower than the regular twinkle mode so people can see it's fading from the previou smode
    delay(15);
    twinkleBeginCtr++;
  }else{
    delay(8);
  }
}

void multiColorWipe(){
  //reusing mode2 variables because they're very similar and won't be used simultaniously anyway
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
    delay(50);
  }
  
  mode2ctr++;
  if(mode2ctr>=stripLength){
    mode2ctr=0;
  }
  if(random(0,10)==0){ //certain percentage of the time, change the color
    //the below is to ensure distinct colors so everything isn't just vaugely pastel
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

//MODIFIER MODES
void twinkleMod(){
  if(setupMode){ //If we ever update stripCopy EVERY time we update the strip, we should change this to start at the previous mode and twinkle out from there instead of filling with random
    twinkleBeginCtr=0;
    for(int i=0;i<stripLength;i++){
      stripSecondCopyRed[i]=stripCopyRed[i];
      stripSecondCopyGreen[i]=stripCopyGreen[i];
      stripSecondCopyBlue[i]=stripCopyBlue[i];
    }
  }
  
  for(int i=0;i<stripLength;i++){
    //uint8_t ra =(strip.getPixelColor(i) >> 16); //DON'T USE THIS FUNCTION, REFERENCE GLOBAL COPY INSTEAD
    //uint8_t ga =(strip.getPixelColor(i) >> 8);
    //uint8_t ba =(strip.getPixelColor(i));

    int r=stripCopyRed[i];
    int g=stripCopyGreen[i];
    int b=stripCopyBlue[i];

    uint16_t initialR=stripSecondCopyRed[i];
    uint16_t initialG=stripSecondCopyGreen[i];
    uint16_t initialB=stripSecondCopyBlue[i];

    if(r%2==0){ //if even, go up
      r+=2;
    }else{ //if odd, go down
      r-=2;
    }
    if(g%2==0){ //if even, go up
      g+=2;
    }else{ //if odd, go down
      g-=2;
    }
    if(b%2==0){ //if even, go up
      b+=2;
    }else{ //if odd, go down
      b-=2;
    }

    r = constrain(r,int(initialR*0.7),int(initialR*1.3)); //keep within 30% difference of original strip colors
    g = constrain(g,int(initialG*0.7),int(initialG*1.3));
    b = constrain(b,int(initialB*0.7),int(initialB*1.3));

    int dirSwitch=20;
    if(random(0,dirSwitch)==0){ //certain percentage of the time, add 1 which switches the direction
      r++;
    }
    if(random(0,dirSwitch)==0){
      g++;
    }
    if(random(0,dirSwitch)==0){
      b++;
    }

    r = constrain(r,2,253); //keep within range if it goes outside (NOT 0 to 255)
    g = constrain(g,2,253);
    b = constrain(b,2,253);

    strip.setPixelColor(i,strip.Color(r,g,b));
    stripCopyRed[i]=r;
    stripCopyGreen[i]=g;
    stripCopyBlue[i]=b;
  }
  strip.show();
  if(twinkleBeginCtr<50){ //make the initial transition slower than the regular twinkle mode so people can see it's fading from the previou smode
    delay(15);
    twinkleBeginCtr++;
  }else{
    delay(8);
  }
}


//GITHUB FIRMWARE UPDATE
void firmwareUpdate(void) {
  WiFiClientSecure client;
  //WiFiClientSecure * client = new WiFiClientSecure;
  //client.setCACert(rootCACertificate);
  client.setInsecure(); //prevents having the update the CA certificate periodically (it expiring breaks github updates which SUCKS cause you have to update each ornament manually with the new certificate
  httpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    inCaptivePortal=true;
    Serial.println("HELP: This probably means the ESP is stuck in a captive portal. Make sure it is registered on the network.");
    Serial.print("Your MAC address for registration is ");
    Serial.println(WiFi.macAddress());
    //light strip orange to indicate captive portal
    for(int i=0;i<100;i++){
      if(i%5==0){
        stripUpdate(i,255/2,100/2,0);
      }else{
        strip.setPixelColor(i, strip.Color(0,0,0));
      }
      strip.show();
      delay(10);
    }
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
    //client -> setCACert(rootCACertificate);
    client->setInsecure(); //prevents having the update the CA certificate periodically (it expiring breaks github updates which SUCKS cause you have to update each ornament manually with the new certificate

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
        if(httpCode==-1 || httpCode==-101){
          inCaptivePortal=true;
          Serial.println("HELP: This probably means the ESP is stuck in a captive portal. Make sure it is registered on the network.");
          Serial.print("Your MAC address for registration is ");
          Serial.println(WiFi.macAddress());
          //light strip orange to indicate captive portal
          for(int i=0;i<100;i++){
            if(i%5==0){
              stripUpdate(i,255/2,100/2,0);
            }else{
              strip.setPixelColor(i, strip.Color(0,0,0));
            }
            strip.show();
            delay(10);
          }
        }
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
    }else if(payload.length()>10){ //in captive portal sometimes it just returns a long string of HTML here instead of the firmware version
      inCaptivePortal=true;
      Serial.println("HELP: This probably means the ESP is stuck in a captive portal. Make sure it is registered on the network.");
      Serial.print("Your MAC address for registration is ");
      Serial.println(WiFi.macAddress());
      //light strip orange to indicate captive portal
      for(int i=0;i<100;i++){
        if(i%5==0){
          stripUpdate(i,255/2,100/2,0);
        }else{
          strip.setPixelColor(i, strip.Color(0,0,0));
        }
        strip.show();
        delay(10);
      }
    }else 
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

void stripUpdateHSV(int pixel, uint32_t c){
  int r;
  int g;
  int b;
  r = (uint8_t)(c >> 16),
  g = (uint8_t)(c >>  8),
  b = (uint8_t)c;
  stripUpdate(pixel,r,g,b);
}
