# Mitsubishi-Aircon-SmartThings
Arduino sketch (.ino) and SmartThings device handler to control Mitsubishi air conditioners via Samsung SmartThings.

#**THIS PROJECT IS STILL WORK IN PROGRESS**

# What can you do with this project?
This project is for Mitsubishi air conditioner owners which want to control their aircons via Samsung SmartThings.
There are a lot of projects out there which use e.g. MQTT but I wanted just simply control my Mitsubishi air conditioners direct from my Samsung SmartThings Classic app.

# How it works (Overview)?
1. You need to use the provided device handler of this project and add the related SmartThings device.
2. The device handler makes standard http calls to the web server running on the ESP32 to control the air conditioner.
Some of you might raise their hands now and shout "Ohh no, that's not secure!". You are right but the communication is only happening in your local network as only your SmartThings hub is normally direct talking to your device. 
The web server on the ESP32 also serves a small root web page just showing you the current air conditioner status plus the latest log output and aalso enables you to reboot the ESP32 if it's required.

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
10. Some kind of housing if you don't want to place the components direct in your aircon.
11. A little bit soldering knowledge.

So you will end up with a bunch of parts costing you normally not more than 10 USD, e.g. like this:
![Parts List](https://github.com/JMan7777/Mitsubishi-Aircon-SmartThings/blob/master/Parts.jpg)
"Your parts and amounts might vary based on your final assambly"

# Connection Diagram
Assemble your parts according to the diagram. 
**_Attention:_** Before connectiong the 5 wires comming from your air conditoner to your assembly, always check the wiring via a multimeter to ensure it's correct. Especially check for 12V, 5V and GND!
![Connection Diagram](https://github.com/JMan7777/Mitsubishi-Aircon-SmartThings/blob/master/Connection_Diagramm.jpg)

# Comments
You might be wondering on the following things:
1. Why I'm not using the TX/RX pins on the ESP32 to connect the air conditioner to. My code uses also serial console output for debugging/logging and the console shares the Serial port. I wanted to prevent any interference and using therefore seperate pins.
2. Why I'm using not the 5V from the air conditioner to power the ESP32 directly. While it most likely works to power the ESP32 directly via 5V I decided to be better save than sorry and draw the power from 12V as there are no technical descriptions available on how much power can be drawn from 5V. The original Mitsubishi Wifi dongle also seems to use 12V for powering.

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
