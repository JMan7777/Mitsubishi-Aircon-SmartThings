# Mitsubishi-Aircon-SmartThings
Arduino sketch (.ino) and SmartThings device handler to control Mitsubishi air conditioners via Samsung SmartThings.


# What can you do with this project?
This project is for Mitsubishi air conditioner owners which want to control their aircons via Samsung SmartThings.

# Requirements 
1. A Mitsubishi air conditioner which is supported by the [HeatPump](https://github.com/SwiCago/HeatPump) project.
2. An ESP32 module (e.g. ESP32-DevKitC)
3. A DC-DC Buck Step-down power converter to convert 12V DC to 5V DC. (Don't use AMS1117 converters.)
4. A SSD1306 OLED I2C Display capable to run at 3.3V DC (e.g. 0.91 inch 128x32 dots)
5. 3 LEDs (Red, Yellow, Green) with resistors to restrict their power usage to below 9mA@3.3V DC.
6. 1 Bi-Directional Logic Level Shifter to shift (RX & TX) signals between 5V DC and 3V DC.
7. A JST 2.0 PA plug with wires. (PA plugs are sometimes not easy to get. PH plugs will be fine too - with some tweaking -).
8. A small button touch switch.
9. Some jumper cables and heat shrinking tubing.
10. A little bit soldering knowledge.


# Quick start


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
