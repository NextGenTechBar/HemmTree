# HemmTree
Code to run Gonzaga Hemmingson Christmas Tree ornaments and other LED strips synchronized via wifi with ESP32s

ESP32 WROOM is used in this project. 
It is important that all ESPs are identical, otherwise at bootup they will firmware update with the compiled binary which may be incompatible with the board.

TABLE OF CONTENTS


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

<b>Wrong colors?</b> Some Neopixel LED strips use different color orders. By default, HemmTree sends commands in the Red,Green,Blue (RGB) format. However, if your strip expects commands in, for example, the GRB format, "red" will show as green and "green" will show as red. To correct this, there are special pins you can ground on the microcontroller to instruct it to change how colors are shown.
* <b>RGB: </b>Do not make any additional connections
* <b>BRG: </b>Solder pin D4 permenantly to GND
* <b>GRB: </b>Solder pin D15 permenatly to GND

<p></p>

### FIRMWARE UPDATES
If you want to add new special modes, or make any other changes, GitHub OTA updates let you wirelessly update the code on all ESPs simultaniously.

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

### USAGE
Once connected to a wifi network, each ESP32 receives commands over MQTT from the MQTT server `broker.mqtt-dashboard.com` on the topic GUHemmTree. These commands are have a special syntax as outlined below in the "MQTT Command Formatting" section. The website https://ngtb95.wixsite.com/ngtb is set up to send commands in a user-friendly way using the npm MQTT package in Wix.

### STATUS INDICATIONS
The ESP32 can indicate various things by lighting up the strip in a specific way. If only every 5th LED is on, this is a status indication.

* <b>White:</b> The ESP32 is currently downloading an update from GitHub. DO NOT unplug it. This should only last for around 30 seconds, after which it will reboot with the new firmware. If the strip has been stuck like this for more than 3 minutes, you can try unplugging it and plugging it back in. Possibilities are:
  * There was a glitch and the reboot will fix it
  * It is stuck in an update loop (if so, the strip will alternate from solid white to spotted white). The version number in `ESP32_code.bin` and `code_version.txt` probably do not match
  * Your firmware update has broken something

* <b>Blue:</b> The ESP32 could not connect to the saved network and is now in Configuration Mode. It will retry automatically in 5 minutes, and you can unplug and plug it back in to retry immediately. Otherwise, follow the initial setup instructions above, starting at step 2.
* <b>Orange: </b> The ESP32 could connect to the saved wifi network, but cannot access GitHub. Likely it is stuck in a captive portal and needs registered to your network. If you need the MAC address, you can plug in to the ESP32 USB port, and the MAC address will be printed over Serial Monitor.
  <br>Note: if it only flashes orange breifly on boot, that means it can connect to the MQTT server, but not GitHub. This means it will have full functionality, but OTA updates probably won't work.

### Python programs

### MQTT COMMAND FORMATTING
If you want to add a way to control the lights, you'll need to know how to format the commands. Adding a control source is as easy as programming something (python program, microcontroller, website, etc) to publish MQTT messages to the topic `GUHemmTree` on the server `broker.mqtt-dashboard.com`. There is no limit to how many simultanious sources can send commands. Invalid commands are ignored by the ornaments. You should send the commands with the retain flag set to true, so that newly subscribed clients will know what the other ornaments are displaying and sync up immediately. 

In general, the format is `|TYPE OF COMMAND|RED VALUE 1|GREEN VALUE 1|BLUE VALUE 1|RED VALUE 2|......` and there are no limits to the length of your patterns

* <b>Types of commands</b>
  * <b>Solid Color / Repetition Pattern: </b>COLOR
    * ex: COLOR000255000 - Make the whole strip green
    * ex: COLOR000255000255000000 - Make the first pixel green, the second pixel red, the 3rd pixel green, 4th red, etc...
    * ex: COLOR000255000255000000255255000 - green, red, yellow, green, red, yellow.......
  * <b>Split strip in to equal sections: </b>FRACS
    * ex: FRACS255000000000000255 - Split the strip in to equal parts red and blue
    * ex: FRACS255000000000000255255255000 - One third of the strip red, one third blue, one third yellow
  * <b>Modes with animation: </b>DYNAM
    * These trigger modes pre-programmed in the firmware. If you would like to add a new one, you will have to issue a firmware update (see above).
    * ex: DYNAMrainbow - starts the moving rainbow mode (see code for complete list of commands, starting at `=="DYNAM"`)
  * <b>Modes that have ESP-generated patterns/animations, but end static: </b>OTHER
    * These trigger modes pre-programmed in the firmware. If you would like to add a new one, you will have to issue a firmware update (see above).
    * ex: OTHERdifferentcolors - Each ESP32 picks a random color to display, and stays that color until the next command is given (see code for complete list of commands, starting at `=="OTHER"`)
  * <b>Modes that show a breif animation, then return to the previous mode: </b>SHORT
    * These trigger modes pre-programmed in the firmware. If you would like to add a new one, you will have to issue a firmware update (see above).
    * ex: SHORTthayne - makes each ornament flash Gonzaga colors, then return to the previous mode
    * (see code for complete list of commands, starting at `=="SHORT"`)
  * <b>Pulse the strip x times: </b>PULSE
    * ex: PULSE5 - pulses the ornaments 5 times in whatever mode they are currently in, then returns to that mode (useful for pulsing the top of each hour from a python program)
