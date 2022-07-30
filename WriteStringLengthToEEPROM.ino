/*
PLEASE NOTE: This functionality is now availible to users without using this script. The option is in the Wifi Configuration portal that launches when the device cannot connect to the saved network.
This script will still work to update that value if it's easier for you (with this script you can be in range of a known network)
*/

//Libraries
#include <EEPROM.h>//https://github.com/espressif/arduino-esp32/tree/master/libraries/EEPROM

//Constants
#define EEPROM_SIZE 12

void setup() {
  //Init Serial USB
  Serial.begin(115200);
  Serial.println(F("Initialize System"));
  //Init EEPROM
  EEPROM.begin(EEPROM_SIZE);

  //Write data into eeprom
  int address = 0;
  int boardId = 18;
  EEPROM.write(address, boardId);//EEPROM.put(address, boardId);
  address += sizeof(boardId); //update address value

  float param = 300; // CHANGE THIS VALUE TO WHAT YOU WANT THE STRING LENGTH TO BEEEEEEEEEEEEEEEEEEEEEEEE
  EEPROM.writeFloat(address, param);//EEPROM.put(address, param);
  EEPROM.commit();

  //Read data from eeprom
  address = 0;
  int readId;
  readId = EEPROM.read(address); //EEPROM.get(address,readId);
  Serial.print("Read Id = ");
  Serial.println(readId);
  address += sizeof(readId); //update address value

  float readParam;
  EEPROM.get(address, readParam); //readParam=EEPROM.readFloat(address);
  Serial.print("VALUE STORED IN EEPROM = ");
  Serial.println(readParam);
  if(isnan(readParam)){
    Serial.println("no stored eeprom val");
  }

  EEPROM.end();
}

void loop() {}
