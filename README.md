# HemmTree
Code to run Gonzaga Hemmingson Christmas Tree ornaments and other LED strips synchronized via wifi with ESP32s

ESP32 WROOM is used in this project. 
It is important that all ESPs are identical, otherwise at bootup they will firmware update with the compiled binary which may be incompatible with the board.

<b>INITIAL SETUP (done individually for each ESP</b>
  1) Setting the number of LEDs this ESP will run (optional)<br>
        If you skip this step, the ESP will use the default strip length as specified in the code. If you would like to use a different strip length, download WriteStringLengthToEEPROM.ino and change the line 'float param = 300;', replacing 300 with the desired strip length. Upload the code to the ESP. The strip length is now set in ROM.
  2) Flashing the initial ESP code
        Download HemmTree2021.ino, place it in the same directory as cert.h and upload it to the ESP32 over USB. It will now check for firmware updates from this repository each boot.
  

<b>FIRMWARE UPDATES</b>
  1) Modifying the code
        If you would like to deploy updates to all ESPs, make your modifications to the HemmTree2021.ino code, then increment FirmwareVer in code.
  2) Compiling the code
        In the Arduino IDE, choose Sketch-->Export compiled binary. Rename the exported file to ESP32_code.bin and upload it to this GitHub repository to replace the current version
  3) Updating version number
        Update the number in the file code_version.txt in this repository to reflect the number you just incremented FirmwareVer to in the code.
  4) Commanding the firmware update
        At the next reboot, each ESP32 will compare the current FirmwareVer number to the one on GitHub, then download and flash the new .bin file if the GitHub number is greater than the local one. If you would like to issue a firmware update immediately, you can send the command 'FIRMWARE_UPDATE' to the topic ('GUHemmTree' at the time of writing), over MQTT and this will instruct all online ESP32s to check for a new firmware version. Note: you can manually issue MQTT instructions via http://www.hivemq.com/demos/websocket-client/

<b>USAGE</b><br>
At the time of writing, the webserver/python code does not exist yet. But in general, all ESP32s will receive commands via MQTT. At the time of writing, the broker is broker.mqtt-dashboard.comt and the topic is GUHemmTree (you can check the code to verify this). Please check the code for the latest possible commands, but in general, the format is:<br>
  |TYPE OF COMMAND|RED VALUE FOR PIXEL 1|GREEN VALUE FOR PIXEL 1|BLUE VALUE FOR PIXEL 1|RED VALUE FOR PIXEL 2|......<br>
  for example, COLOR000255000000000255 tells the ESP to turn the first light green, then the second light blue, then repeat that pattern for the whole strip (note, adding 9 more digits makes it a three digit pattern)<br>
  FRACS000255000000000255 tells it to divide the strip in to equal parts green and blue<br>
  DYNAMRAINBOW tells it to execute the "Rainbow" animation.
  
  MQTT messages can be generated from any, or multiple sources. So for example, you can have a webserver accepting user input and sending the results over MQTT int he proper format. You can have a python program listening for a twitter hashtag, decoding the twitter message and sending the result over MQTT. You can even have a seperate python program running to send messages such as pulsing the lights at the top of each hour, changing the color when someone stands in a specific place, etc. You can come up with as many input sources as you want!
