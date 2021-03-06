# Mitsubishi-Aircon-SmartThings
Arduino sketch (.ino) and SmartThings device handler to control Mitsubishi air conditioners via Samsung SmartThings.

# Attention: The SmartThings Classic integration part of this project will not be further developed as Samsung stopped the SmartThings Classic app. As most of my other own devices are not fully functioning in the new SmartThings app as well, I'm currently migrating all my sensors, etc. to Home Assistant. The ESP32 Arduino Sketch of this project will be further maintained and I will create a new project to integrate the ESP32 into Home Assistant.

# What can you do with this project?
This project is for Mitsubishi air conditioner owners which want to control their aircons via Samsung SmartThings.
There are a lot of projects out there which use e.g. MQTT but I wanted just simply control my Mitsubishi air conditioners direct from my Samsung SmartThings Classic app.

# How it works (Overview)?
1. You need to use the provided device handler of this project and add the related SmartThings device.
2. The device handler makes standard http calls to the web server running on the ESP32 to control the air conditioner.
Some of you might raise their hands now and shout "Ohh no, that's not secure!". You are right but the communication is only happening in your local network as only your SmartThings hub is normally direct talking to your ESP32. 
The web server on the ESP32 also serves a small root web page just showing you the current air conditioner status plus the latest log output and aalso enables you to reboot the ESP32 if it's required.
3. I also added a small OLED display and 3 LED's to show the status of the air conditioner and some other useful information. A reboot button for the ESP32 completes the setup.

_The OLED display and 3 LED's are actually optional. So even without it this project should work for you._

# Requirements 
1. A Mitsubishi air conditioner which is supported by the [HeatPump](https://github.com/SwiCago/HeatPump) project.
2. An ESP32 module. Can have integrated or external antenna. (e.g. ESP32-DevKitC)
3. A DC-DC Buck Step-down power converter to convert 12V DC to 5V DC. (Don't use AMS1117 converters.)
4. A SSD1306 OLED I2C Display capable to run at 3.3V DC. (e.g. 0.91 inch 128x32 dots)
5. 3 LEDs (Red, Yellow, Green) with resistors to restrict their power usage to below 9mA @ 3.3V DC.
6. 1 Bi-Directional Logic Level Shifter to shift (RX & TX) signals between 5V DC and 3V DC.
7. A JST 2.0 PA plug (5-pin) with wires. Unfortunately PA plugs are sometimes not easy to get. I used PH plugs with some tweaking. **Attention:** PH plugs must be inserted to the air conditioner side CN105 connector 180 degrees turned otherwise they don't fit!
8. A small button touch switch.
9. Some jumper cables and heat shrinking tubing.
10. Some kind of housing if you don't want to place the components direct in your air conditioner. Even you could fit all the parts in the little detachable corner, L-shape plastic piece on the right bottom site of your air conditioner, I decided to put mine external.
11. A little bit soldering knowledge.

_The OLED display and 3 LED's are actually optional. So even without it this project should work for you._

So you will end up with a bunch of parts costing you normally not more than 10 USD, e.g. like this:

<p align="center">
  <img src="https://github.com/JMan7777/Mitsubishi-Aircon-SmartThings/blob/master/pictures/Parts.jpg" width="400">
</p>

"Your parts and amounts might vary based on your final assembly"

# Connection Diagram
Assemble your parts according to the diagram. 

**_Attention:_** Before connectiong the 5 wires coming from your air conditoner to your assembly, always check the wiring via a multimeter to ensure it's correct. Especially check for 12V, 5V and GND!

<p align="center">
  <img src="https://github.com/JMan7777/Mitsubishi-Aircon-SmartThings/blob/master/pictures/CN105_Pins.jpg" width="150">
</p>

<p align="center">
  <img src="https://github.com/JMan7777/Mitsubishi-Aircon-SmartThings/blob/master/pictures/Connection_Diagramm.jpg" width="400">
</p>

If you are using a PH connector (with some trimming to make it smaller), the plug needs to be 180 degree turned to fit the CN105 socket (in my example below the black wire at pin 5 = RX)

<p align="center">
  <img src="https://github.com/JMan7777/Mitsubishi-Aircon-SmartThings/blob/master/pictures/CN105_PH.jpg" width="400">
</p>

_The OLED display and 3 LED's are actually optional. So even without it this project should work for you._

# Comments
You might be wondering on the following things:
1. Why I'm not using the TX/RX pins on the ESP32 to connect the air conditioner to. My code uses also serial console output for debugging/logging and the console shares the Serial port. I wanted to prevent any interference and using therefore seperate pins.
2. Why I'm using not the 5V from the air conditioner to power the ESP32 directly. While it most likely works to power the ESP32 directly via 5V I decided to be better save than sorry and draw the power from 12V as there are no technical descriptions available on how much power can be drawn from 5V. The original Mitsubishi Wifi dongle also seems to use 12V for powering.

# ESP32 Arduino Sketch
Upload the following sketch using the Arduino IDE.
Please ensure you include all required libraries and update the environment specific settings.

![Mitsubishi_Aircon_ESP32.ino](https://github.com/JMan7777/Mitsubishi-Aircon-SmartThings/blob/master/ino/Mitsubishi_Aircon_ESP32.ino)

By default your ESP32 will aquire its IP address via DHCP. After bootup you can access the ESP32 webserver status page simply via:
<dl>
  <dd>http://YOUR-IP-ADDRESS/</dd>
</dl>

The following is an example of sending a POWER and FAN change to the air conditioner. The webserver status page shows this as follows:

<p align="center">
  <img src="https://github.com/JMan7777/Mitsubishi-Aircon-SmartThings/blob/master/pictures/Root_Page.jpg" width="400">
</p>

The JSON sent via a POST request to:

<dl>
  <dd>http://YOUR-IP-ADDRESS/ChangeAirconSettings</dd>
</dl>

was in this case:

```
{
  "POWER": "ON",
  "FAN": "1"
}
```

In the same way the SmartThings Device Handler will communicate changes to the air conditioner.

To get the current settings via JSON the following request can be made:

<dl>
  <dd>http://YOUR-IP-ADDRESS/GetAirconSettings</dd>
</dl>

which results in:

```
{
  "POWER": "ON",
  "MODE": "COOL",
  "TEMP": "25",
  "FAN": "1",
  "V_SWING": "SWING",
  "H_SWING": "|",
  "ROOM_TEMP": "33"
}
```

_By the way, the "Latest serial output:" shown on the webserver status page is also the information you will see on the SSD1306 display._

# SmartThings Device Handler
Login to https://graph.api.smartthings.com/ and install the device handler and create a related device.
Afterwards scan in the SmartThings Classic app for new Things and add the found air conditioner.
Don't forget to set the ESP32 IP address, port and your temperature unit in the air conditioner Things settings.

![Mitsubishi_Aircon_Device_Handler.groovy](https://github.com/JMan7777/Mitsubishi-Aircon-SmartThings/blob/master/groovy/Mitsubishi_Aircon_Device_Handler.groovy)

<p align="center">
  <img src="https://github.com/JMan7777/Mitsubishi-Aircon-SmartThings/blob/master/pictures/SmartThings_Android.jpg" width="400" >
</p>

There are also some limitations at the moment:
1. The [HeatPump](https://github.com/SwiCago/HeatPump) library takes few seconds until it delivers back just updated settings. In case you refresh quickly after triggering an air conditioner change, you might still see the unchanged settings.
2. Some commands might not be physically possible by your air conditioner. If e.g. your air conditioner has no heat mode, the command to change to heat mode will be accepted by the air conditioner but nothing will change. A refresh afterward will show you back the old settings. 

_The device handler is still not pretty and WIP (source code comments e.g. missing, the auto refresh is not yet alligned with updates, limitations above, etc.)._

# Prototype
Since I don't have a 3D printer (yet ;)) I just placed all components in a standard socket housing.
Another posibility is to place the components in the little detachable corner, L-shape plastic piece on the right bottom site of your air conditioner.

<p align="center">
  <img src="https://github.com/JMan7777/Mitsubishi-Aircon-SmartThings/blob/master/pictures/Prototype.jpg" width="400" >
</p>

<p align="center">
  <img src="https://github.com/JMan7777/Mitsubishi-Aircon-SmartThings/blob/master/pictures/Prototype_Inside.jpg" width="400" >
</p>

# Special thanks
... to SwiCago for providing the Arduino library to control Mitsubishi Heat Pumps via connector CN105:

https://github.com/SwiCago/HeatPump

# License
The code written as part of this project is licensed under the GNU Lesser General Public License:

https://www.gnu.org/licenses/lgpl.html

However, license T&C of 3rd party libraries used by this project are applicable.

# Disclaimer - "Use at your own risk!"
Despite having reasonable technical expertise I'm giving absolutely no guarantee that the projects / tutorials / any piece information presented in this project website are 100% correct, but best efforts has been made to minimize the errors and I believe the project present on this website works. Users of this project must do their research and understand the concepts to the core before proceeding with the given project.
I cannot be held responsible for any damages in the form of physical loss / monetary loss or any kind of loss that comes as the result of making use of information and instructions that are presented in this website.
