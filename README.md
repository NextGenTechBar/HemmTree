# HemmTree
Code to run Gonzaga Hemmingson Christmas Tree ornaments and other LED strips synchronized via wifi with ESP32s

ESP32 WROOM is used in this project. 
It is important that all ESPs are identical, otherwise at bootup they will firmware update with the compiled binary which may be incompatible with the board.

### INITIAL SETUP (done individually for each ESP)
This assumes you already have the Arduino IDE installed with ESP32 boards. If not, please follow instructions <a href="https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/">here</a> first. You may also need <a href="https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers">this</a> driver.
<ol>
  <li><b>Flashing the initial ESP code</b><br>
    The full HemmTree2021.ino code has many library dependancies, so it is easiest to flash firmwareUpdate.ino, which has no external dependancies. This code will connect to wifi and immediately download+install the full code from this repository and reboot. To use it, just change the SSID and password at the top of the sketch to your wifi details. Then upload it to your ESP32. The onboard blue light will flash rapidly to indicate it is downloading the full code.
    <br>[when uploading code, use the board "Node32s"]
    <br>[If the blue light does not rapidly flash, you have probably entered the wrong WiFi credentials, or the device is stuck in a captive portal]
  </li>
  <li><b>Connecting to the configuration network</b><br>
    When the download is finished, the ESP32 will reboot with the full HemmTree code. The LED strip will light up white, and then change to spotted blue (black except every 5th light blue) to indicate that the configuration portal has started. To access it, from your computer or phone, connect to the wifi network called "HemmTree ESP-[MAC]". On most computers and iphones, this will automatically launch a captive portal. On Androids, you may need to go to 192.168.4.1 in your browser.
    <br>[Note: the configuration portal may not launch if this ESP has been used on an in-range network before. It may connect to that network automatically instead.]
  </li>
  <li><b>Selecting wifi network and strip length</b><br>
    Once the configuration page has loaded, select "Configure WiFi". On the following screen you can select the desired WiFi network and enter the Strip Length, which indicates how many individual series LEDs are on that ornament. Once finished, press save. After a few seconds (assuming valid WiFi credentials were entered), it will connect to that wifi network and begin normal functionality. If it displays spotted orange, the device is likely stuck in a captive portal on your chosen network.
    <br>From now on, when plugged in this device will automatically connect to the saved wifi network. If that fails at boot, it will launch the configuration network again (as indicated by spotted blue), and you can select a new wifi network.
  </li>
</ol>

<p></p>

### FIRMWARE UPDATES
<br>If you want to add new special modes, or make any other changes, GitHub OTA updates let you wirelessly update the code on all ESPs simultaniously.

  1) <b>Modifying the code</b><br>
        If you would like to deploy updates to all ESPs, make your modifications to the HemmTree2021.ino code, then increment FirmwareVer in code. (BE CAREFUL! If you accidentally make a change that breaks the wifi connection code or firmware update code, you could cause all ESPs to become inaccessible and need re-flashed over USB. It is highly recommended to test the new code by uploading it over USB to a test ornament (with old FirmwareVer variable) before deploying it OTA to all ornaments.)
  2) <b>Compiling the code</b><br>
        In the Arduino IDE, choose Sketch-->Export compiled binary. Rename the exported file to ESP32_code.bin and upload it to this GitHub repository to replace the current version
  3) <b>Updating version number</b><br>
        Update the number in the file code_version.txt in this repository to reflect the number you just incremented FirmwareVer to in the code.
  4) <b>Commanding the firmware update</b><br>
        At the next reboot, each ESP32 will compare the current FirmwareVer number to the one on GitHub, then download and flash the new .bin file if the GitHub number is greater than the local one. During a firmware update, the LED strip will display spotted white. DO NOT unplug it during this time. If you would like to issue a firmware update immediately, you can send the command 'FIRMWARE_UPDATE' to the topic ('GUHemmTree' at the time of writing), over MQTT and this will instruct all online ESP32s to check for a new firmware version. Note: you can manually issue MQTT instructions via http://www.hivemq.com/demos/websocket-client/
        

### CHANGING STRIP LENGTH

The strip length (number of series LEDs on each individual ornament) can be updated individually on each ESP32 thorugh the configuration portal. However, the configuration portal is only launched when the ESP32 fails to connect to wifi. Therefore, the portal can be launched by booting the ESP32 outside of range of the saved wifi network. If this is not feasible, you can update the strip length as follows:
<br>Download WriteStringLengthToEEPROM.ino and change the line 'float param = 300;', replacing 300 with the desired strip length. Upload the code to the ESP32 (if you are having difficulties with this step, see the note at the begining of "initial setup" instructions). The new strip length is now set in EEPROM. After this, you can follow the above instructions for "INITIAL SETUP" to put the HemmTree code back on the ESP32.

### USAGE<br>

https://ngtb95.wixsite.com/ngtb

At the time of writing, the webserver/python code does not exist yet. But in general, all ESP32s will receive commands via MQTT. At the time of writing, the broker is broker.mqtt-dashboard.comt and the topic is GUHemmTree (you can check the code to verify this). Please check the code for the latest possible commands, but in general, the format is:<br>
  |TYPE OF COMMAND|RED VALUE FOR PIXEL 1|GREEN VALUE FOR PIXEL 1|BLUE VALUE FOR PIXEL 1|RED VALUE FOR PIXEL 2|......<br>
  for example, COLOR000255000000000255 tells the ESP to turn the first light green, then the second light blue, then repeat that pattern for the whole strip (note, adding 9 more digits makes it a three digit pattern)<br>
  FRACS000255000000000255 tells it to divide the strip in to equal parts green and blue<br>
  DYNAMRAINBOW tells it to execute the "Rainbow" animation.
  
  MQTT messages can be generated from any, or multiple sources. So for example, you can have a webserver accepting user input and sending the results over MQTT int he proper format. You can have a python program listening for a twitter hashtag, decoding the twitter message and sending the result over MQTT. You can even have a seperate python program running to send messages such as pulsing the lights at the top of each hour, changing the color when someone stands in a specific place, etc. You can come up with as many input sources as you want!
