/*
  Mitsubishi_Aircon_ESP32.ino - Mitsubishi-Aircon-SmartThings Arduino Sketch

  This sketch is primary using the HeadPump library from
  https://github.com/SwiCago/HeatPump to communicate with the aircon.

  Additionally is supports:
  - displaying messages on a SSD1306 monochrome display (black / white)
  - uses an NTP server for time 
  - implements Arduino OTA updates
  - 3 LEDs connected via source current

  If you don't have LEDs or a SSD1306 display connected, the code
  should run as well. 

  The implemented async webserver responds to API calls from your
  Samsung Smartthings Hub local LAN calls via HubAction.
  API requests and responses are in JSON format.
  
  Copyright (c) 2020 Markus Jessl.  All right reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//******************************************************************************
// FREERTOS related settings
//
// ESP32 has two cores. We will need to run certain functions on a specific
// core. E.g. the Adafruit_SSD1306 display must run on core 0.
// Since we set all configured tasks to have the same priority, we need to
// enable time slicing.
//******************************************************************************

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#define ARDUINO_NONRUNNING_CORE 1
#else
#define ARDUINO_RUNNING_CORE 1
#define ARDUINO_NONRUNNING_CORE 0
#endif
#define configUSE_TIME_SLICING 1

//******************************************************************************
// Include required libraries.
//
// Libraries which cannot be downloaded with the Arduino Library Manager are
// marked with the related GitHub URL link.
// All other libraries are either included automatically in Arduino or can be
// downloaded via the Arduino Library Manager.
//******************************************************************************

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <AsyncTCP.h>          //https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h> //https://github.com/me-no-dev/ESPAsyncWebServer
#include <AsyncJson.h>         //https://github.com/me-no-dev/ESPAsyncWebServer
#include <HeatPump.h>          //https://github.com/SwiCago/HeatPump
#include <ArduinoJson.h>
#include <NTPClient.h>         //https://github.com/arduino-libraries/NTPClient
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <CircularBuffer.h>    //https://github.com/rlogiacco/CircularBuffer
#include <Ticker.h>

//******************************************************************************
// Environment specific settings.
//
// Adjust these settings according your environment and setup.
// Setting marked with a * in the comment most likely you need to change.
//******************************************************************************

#define MA_DEF_WIFI_SSID "SSID"                  // Your WiFi SSID *
#define MA_DEF_WIFI_PASSWORD "abcdefg"           // Your WiFi password *
#define MA_DEF_WIFI_HOSTNAME "AIRCON01"          // Hostname for this ESP32 *
#define MA_DEF_OTA_PASSWORD "abcdefg"            // Password for OTA updates *
#define MA_DEF_NTP_SERVER "pool.ntp.org"         // NTP server
#define MA_DEF_NTP_SERVER_OFFSET 28800           // TZ offset in sec *
#define MA_DEF_NTP_SERVER_UPDATE_INTERVAL 60000  // NTP update interval in ms
#define MA_DEF_WEBSERVER_PORT 80                 // ESP32 webserver port
#define MA_DEF_SCREEN_WIDTH 128                  // SSD1306 disp. pixel width
#define MA_DEF_SCREEN_HEIGHT 32                  // SSD1306 disp. pixel height
#define MA_DEF_SCREEN_OMA_DEF_LED_RESET -1       // SSD1306 disp. reset pin
const uint8_t MA_CONST_SCREEN_ADDRESS = 0x3C;    // SSD1306 display address
                                                 // 0x3D for 128x64
#define MA_DEF_SCREEN_TEXT_SIZE_2_MAX_LENGTH 10  // SSD1306 disp. maximum number
                                                 // of TextSize 2 characters *
#define MA_DEF_AC_RX_PIN 16                      // RX pin used by Serial2
#define MA_DEF_AC_TX_PIN 17                      // TX pin used by Serial2
#define MA_DEF_AC_USE_CELCIUS true               // Temperature unit C or F *
#define MA_DEF_LED_OFF LOW                       // Pin power state for off
#define MA_DEF_LED_ON HIGH                       // Pin power state for on
#define MA_DEF_LED_RED_PIN 25                    // Red LED pin
#define MA_DEF_LED_YELLOW_PIN 26                 // Yellow LED pin
#define MA_DEF_LED_GREEN_PIN 27                  // Green LED pin
#define MA_DEF_LED_BLINK_INTERVAL 250            // LED blink interval in ms
#define MA_DEF_WIFI_MAX_RECONNECT_COUNT 60       // Max reconnect count if WiFi
                                                 // connection is lost before
                                                 // reboot
//******************************************************************************
// Constant settings.
//
// These constants are used in the JSON communication between the ESP32
// and the Samsung SmartThings device handler. 
//******************************************************************************
  
const String MA_CONST_AC_SETTING_POWER = "POWER";
const String MA_CONST_AC_SETTING_MODE = "MODE";
const String MA_CONST_AC_SETTING_TEMP = "TEMP";
const String MA_CONST_AC_SETTING_FAN = "FAN";
const String MA_CONST_AC_SETTING_V_SWING = "V_SWING";
const String MA_CONST_AC_SETTING_H_SWING = "H_SWING";
const String MA_CONST_AC_STATUS_ROOM_TEMP = "ROOM_TEMP";
const String MA_CONST_AC_STATUS_OPERATING = "OPERATING";
const String MA_CONST_AC_STATUS_COMP_FREQ = "COMP_FREQ";

//******************************************************************************
// Struct declarations.
//******************************************************************************
typedef struct ma_Struct_DisplayMessage {
  int line;
  int delayMS;
  String message;
  int ledPinBlink;
} DisplayMessage;

//******************************************************************************
// Variable declarations.
//******************************************************************************

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 ma_Display(MA_DEF_SCREEN_WIDTH, MA_DEF_SCREEN_HEIGHT,
                            &Wire, MA_DEF_SCREEN_OMA_DEF_LED_RESET);

//Declaration of used task handles
TaskHandle_t ma_DisplayTaskHandle;
TaskHandle_t ma_AirconTaskHandle;

//Declaration ring buffers
CircularBuffer<DisplayMessage, 500> ma_DisplayBuffer;
CircularBuffer<String, 500> ma_SerialOutputBuffer;

//Declaration of the webserver
AsyncWebServer ma_Webserver(MA_DEF_WEBSERVER_PORT);

//Declaration of the air conditioner and related variables
HeatPump         ma_HeadPump;
heatpumpSettings ma_HeadPumpActualSettings, ma_HeadPumpUpdatedSettings;
heatpumpStatus   ma_HeadPumpActualStatus;
bool             ma_HeadPumpConnectedOnStartup = true;
bool             ma_HeadPumpUpdatedSettingsAvailable = false;

//Declaration of NTP server related variables
WiFiUDP         ma_NtpUDP;
NTPClient       ma_TimeClient(ma_NtpUDP, 
                        MA_DEF_NTP_SERVER, 
                        MA_DEF_NTP_SERVER_OFFSET, 
                        MA_DEF_NTP_SERVER_UPDATE_INTERVAL);
String          ma_StartupTimeString;

//Declaration of a variable used to initiate a reboot
bool ma_InitiateReboot = false;

//Declaration of a variable used to count how many WiFi reconnects are done
int ma_WiFiReconnectCount = 0;

//******************************************************************************
// Custom Functions
//******************************************************************************

///*****************************************************************************
///Function name: getNTPTime
///Description:   Returns the current time as epoch.
///               Function is used SyncProvider of the TimeLib library
///Return type:   time_t
///*****************************************************************************
time_t getNTPTime()
{
  //Ensure we get the current time provided by the NTP server
  ma_TimeClient.update(); 
  return ma_TimeClient.getEpochTime();
}

///*****************************************************************************
///Function name: getCurrentTimeAsString
///Description:   Returns the current datetime in format "[DD.MM.YYYY] HH:MI:SS" 
///Return type:   String
///Parameters:    bool timeOnly -> If true only the current time is returned
///*****************************************************************************
String getCurrentTimeAsString(bool timeOnly = false)
{
  String currentTimeString = "";
  
  if (timeOnly == false)
  {
    if (day() < 10)
    {
      currentTimeString = currentTimeString + "0";
    }
    currentTimeString = currentTimeString + (String) day() + ".";

    if (month() < 10)
    {
      currentTimeString = currentTimeString + "0";
    }
    currentTimeString = currentTimeString + (String) month() + "." + 
                                                     year() + " ";
  }
  
  if (hour() < 10)
  {
    currentTimeString = currentTimeString + "0";
  }
  currentTimeString = currentTimeString + hour();
  currentTimeString = currentTimeString + ":";

  if (minute() < 10)
  {
    currentTimeString = currentTimeString + "0";
  }
  currentTimeString = currentTimeString + minute();
  currentTimeString = currentTimeString + ":";

  if (second() < 10)
  {
    currentTimeString = currentTimeString + "0";
  }
  currentTimeString = currentTimeString + second();

  return currentTimeString;
}

///*****************************************************************************
///Function name: writeToOutput
///Description:   Writes a message to the Serial console and to the the
///               ma_SerialOutputBuffer
///Return type:   void
///Parameters:    String message -> Message to be written to the Serial console
///               and to the ma_SerialOutputBuffer
///*****************************************************************************
void writeToOutput(String message)
{
  //Serial console output
  Serial.println(message);
  //If the ring buffer is full remove older entries first.
  while (ma_SerialOutputBuffer.isFull())
  {
    ma_SerialOutputBuffer.shift();
  }
  ma_SerialOutputBuffer.push(message);
}

///*****************************************************************************
///Function name: toggleLEDState
///Description:   Toggles the power state of a pin
///Return type:   void
///Parameters:    int pin -> Pin where the power state should be toggled
///*****************************************************************************
void toggleLEDState(int pin) 
{
  //Get the current state of the GPIO pin
  int state = digitalRead(pin);  
  //Set GPIO pin to the opposite state
  setLEDState(pin, !state);
}

///*****************************************************************************
///Function name: setLEDState
///Description:   Sets the power state of a pin
///Return type:   void
///Parameters:    int pin -> Pin where the power state should be set
///               int newState -> Requested state for the pin
///*****************************************************************************
void setLEDState(int pin, int newState)
{
  //Get the current state of the GPIO pin
  int state = digitalRead(pin);
  //If the current state does not equal the requested state, change the state
  if (state != newState)
  {
    digitalWrite(pin, newState);
  }
}

///*****************************************************************************
///Function name: nonTaskDelay
///Description:   Delay function with LED blink support
///Restriction:   To be only used outside tasks!
///               Usage from setup() or loop() only!
///               Blinking will only occur if 
///               delayMS > MA_DEF_LED_BLINK_INTERVAL.
///Return type:   void
///Parameters:    int delayMS -> Total delay in ms
///               int ledPin -> If > 0 the related pin will blink
///*****************************************************************************
void nonTaskDelay(int delayMS, int ledPin =  0) 
{
  //Check if delay is needed 
  if (delayMS > 0)
  {
    //Check if a ledPin is specified and the delay is long enough to support 
    //blinking
    if (ledPin > 0 && delayMS > MA_DEF_LED_BLINK_INTERVAL)
    {
      //Read the current state of the ledPin
      int state = digitalRead(ledPin);
      //Start a ticker toggeling the ledPin
      Ticker ledTicker;
      ledTicker.attach_ms(MA_DEF_LED_BLINK_INTERVAL, toggleLEDState, ledPin);
      //Wait
      delay(delayMS);
      //Stop the ticker
      ledTicker.detach();
      //Restore the original ledPin state
      setLEDState(ledPin, state);    
    }
    else
    {
      //Wait
      delay(delayMS);
    }
  }
}

///*****************************************************************************
///Function name: clearDisplayLine
///Description:   Clears logical lines the SSD1306 display (makes them black)
///Restriction:   This project only supports to write 2 separate lines to the
///               display. This meanbs line 1 will be using the first height
///               half of the display while line 2 uses the second half.
///Return type:   void
///Parameters:    int line -> Line to be cleared (Can only be 1 or 2)
///               bool refreshDisplay -> If display buffer should be displayed
///*****************************************************************************
void clearDisplayLine(int line = 1, bool refreshDisplay = false) 
{
  //Declare local variables
  bool doClear = true;
  int yStart = -1;

  //Set the physical y-axis start line for the clearing
  if (line == 1)
  {
    yStart = 0;
  }
  else if (line == 2)
  {
    yStart = (MA_DEF_SCREEN_HEIGHT / 2);
  }
  else
  {
    doClear = false;
    writeToOutput("Display line " + String(line) + 
                  " is invalid. Cannot clear line."); 
  } 

  if (doClear)
  {
    //Clear the display by creating a filled black rectangle
    ma_Display.fillRect(0, 
                        yStart, 
                        MA_DEF_SCREEN_WIDTH, 
                        (MA_DEF_SCREEN_HEIGHT / 2),
                        BLACK);

    //Refresh the display immediately if requested
    if (refreshDisplay)
    {
      ma_Display.display();
    }
  }
}

///*****************************************************************************
///Function name: writeDisplayMessage
///Description:   Writes a DisplayMessage to the display line 1 or 2
///Restriction:   Only call this function outside of setup() or loop()
///               if you have specified a DisplayMessage.delayMS = 0.
///               If you need a delay, use queueDisplayMessage instead.
///Return type:   void
///Parameters:    DisplayMessage message -> DisplayMessage to be displayed
///               bool writeToOutputToo -> Write DisplayMessage also via
///                                        writeToOutput function
///*****************************************************************************
void writeDisplayMessage(DisplayMessage message, bool writeToOutputToo = true) 
{
  //Clear the display line
  clearDisplayLine(message.line);  

  //Sets the correct text size
  if (message.message.length() > MA_DEF_SCREEN_TEXT_SIZE_2_MAX_LENGTH)
  {
    ma_Display.setTextSize(1);
  }
  else
  {
    ma_Display.setTextSize(2);
  }

  //Writes the message also via the writeToOutput function if requested
  if (writeToOutputToo)
  {
    writeToOutput(message.message);
  }

  //Sets the display cursor according to line
  if (message.line == 1)
  {
    ma_Display.setCursor(0, 0);
  }
  else if (message.line == 2)
  {
    ma_Display.setCursor(0, (MA_DEF_SCREEN_HEIGHT / 2));
  }
  else
  {
    writeToOutput((String) "Display line " + message.line + " is invalid. " +
                           "Specified for message: '" + message.message + 
                           "'.");
    ma_Display.setCursor(0, 0);
  }

  //Displays the message
  ma_Display.println(message.message);
  ma_Display.display();

  //Waits if a message.delayMS > 0 is specified.
  if (message.delayMS > 0)
  {
    nonTaskDelay(message.delayMS, message.ledPinBlink);
  }
}

///*****************************************************************************
///Function name: writeDisplayMessage
///Description:   Created a DisplayMessage to be displayed.
///               This is a wrapper function for the writeDisplayMessage above.
///Restriction:   Only call this function outside of setup() or loop()
///               if you have specified delayMS = 0.
///               If you need a delay, use queueDisplayMessage instead.
///Return type:   void
///Parameters:    int line -> Line to be written on the display
///               int delayMS -> Wait time in ms after the message have been
///                              displayed
///               String message -> Message to be displayed on the display line
///               int ledPinBlink -> LED pin to blink (0 if no blinking)
///*****************************************************************************
void writeDisplayMessage(int line = 1, int delayMS = 0, String message = " ", 
                         int ledPinBlink = 0)
{
  //Create a DisplayMessage and call the related writeDisplayMessage function
  DisplayMessage newMessage = { line, delayMS, message, ledPinBlink};
  writeDisplayMessage(newMessage);
}


///*****************************************************************************
///Function name: queueDisplayMessage
///Description:   Created a DisplayMessage to be displayed and add it to the
///               related ma_DisplayBuffer.
///Return type:   void
///Parameters:    int line -> Line to be written on the display
///               int delayMS -> Wait time in ms after the message have been
///                              displayed
///               String message -> Message to be displayed on the display line
///               int ledPinBlink -> LED pin to blink (0 if no blinking)
///               bool writeToOutputToo -> Write DisplayMessage also via
///                                        writeToOutput function
///*****************************************************************************
void queueDisplayMessage(int line = 1, int delayMS = 0, String message = " ", 
                         int ledPinBlink = 0, bool writeToOutputToo = true) 
{
  //Writes the message also via the writeToOutput function if requested
  if (writeToOutputToo)
  {
    writeToOutput(message);
  }

  //Create the DisplayMessage
  DisplayMessage nextMessage = {line, delayMS, message, ledPinBlink};

  //If the ring buffer is full remove older entries first
  while (ma_DisplayBuffer.isFull())
  {
    ma_DisplayBuffer.shift();
  }

  //Add the DisplayMessage to the ring buffer
  ma_DisplayBuffer.push(nextMessage);
}

///*****************************************************************************
///Function name: queueDisplayStatusUpdate
///Description:   Queues DisplayMessages with the current time and Aircon 
///               settings to line 1 and 2 of the display.
///Return type:   void
///Parameters:    int delayMS -> Wait time in ms after the message have been
///                              displayed
///               bool writeToOutputToo -> Write DisplayMessage also via
///                                        writeToOutput function
///*****************************************************************************
void queueDisplayStatusUpdate(int delayMS = 0, bool writeToOutputToo = true) 
{
  //Display the current time on line 1 of the display
  queueDisplayMessage(1, 0, getCurrentTimeAsString(true), 0,  writeToOutputToo);
  //Display the aircon settings on line 2 of the display
  queueDisplayMessage(2, delayMS, 
     (String) ma_HeadPumpActualSettings.power 
     //{"OFF", "ON"}
     + "," +
     ma_HeadPumpActualSettings.mode 
     //{"HEAT", "DRY", "COOL", "FAN", "AUTO"}
     + "," +
     ((ma_HeadPumpActualStatus.roomTemperature != NULL && 
       ma_HeadPumpActualStatus.roomTemperature > 0) ? 
          String(
              (
                MA_DEF_AC_USE_CELCIUS ?
                ma_HeadPumpActualStatus.roomTemperature :
                ma_HeadPump.CelsiusToFahrenheit(
                    ma_HeadPumpActualStatus.roomTemperature)
               )
            ).substring(0, 2) :
          "?"
          ) 
     + (MA_DEF_AC_USE_CELCIUS ? "C" : "F")  
     //Current room temperature
     + "->" +
      ((ma_HeadPumpActualSettings.temperature != NULL && 
        ma_HeadPumpActualSettings.temperature > 0) ? 
          String(
              (
                MA_DEF_AC_USE_CELCIUS ?
                ma_HeadPumpActualSettings.temperature :
                ma_HeadPump.CelsiusToFahrenheit(
                  ma_HeadPumpActualSettings.temperature)
               )
            ).substring(0, 2) :
          "?"
          ) 
     + (MA_DEF_AC_USE_CELCIUS ? "C" : "F")  
     //{"31".."16"} C                         
     + "," +
     "FAN=" + ma_HeadPumpActualSettings.fan 
     //{"AUTO", "QUIET", "1", "2", "3", "4"}
     + "," +
     "V=" + ma_HeadPumpActualSettings.vane 
     //{"AUTO", "1", "2", "3", "4", "5", "SWING"}
     + "," +
     "H=" + ma_HeadPumpActualSettings.wideVane 
     //{"<<", "<", "|", ">", ">>", "<>", "SWING"}
     + "," +
     "O=" + ((ma_HeadPumpActualStatus.operating != NULL) ? 
              String(ma_HeadPumpActualStatus.operating ? "Y" : "N"): "?")
     // If true, the air conditioner is operating to reach the desired temperature              
     + "," +
     "F=" + ((ma_HeadPumpActualStatus.compressorFrequency != NULL) ? 
        String(ma_HeadPumpActualStatus.compressorFrequency): "?")
     //Compressor Frequency
     , 0, writeToOutputToo);
}

///*****************************************************************************
///Function name: SendAirconSettingsJSON
///Description:   Creates and sends a JSON response to webserver request
///               containing the given heatpumpSettings.
///Return type:   void
///Parameters:    AsyncWebServerRequest *request -> Webserver request
///               heatpumpSettings settings -> heatpumpSettings to be send
///                                            as JSON response
///*****************************************************************************
void SendAirconSettingsJSON(AsyncWebServerRequest *request, 
                          heatpumpSettings settings = ma_HeadPumpActualSettings) 
{
  //Create the JSON document and fill it with the heatpumpSettings
  DynamicJsonDocument doc(1024);
  //{"OFF", "ON"}
  doc[MA_CONST_AC_SETTING_POWER] = settings.power; 
  //{"HEAT", "DRY", "COOL", "FAN", "AUTO"}
  doc[MA_CONST_AC_SETTING_MODE] = settings.mode; 
  //{"31".."16"} C
  doc[MA_CONST_AC_SETTING_TEMP] = ((settings.temperature != NULL && 
                                    settings.temperature > 0) ? 
                                    String(settings.temperature).substring(0,2):
                                    "?"); 
  //{"AUTO", "QUIET", "1", "2", "3", "4"}
  doc[MA_CONST_AC_SETTING_FAN] = settings.fan;
  //{"AUTO", "1", "2", "3", "4", "5", "SWING"}
  doc[MA_CONST_AC_SETTING_V_SWING] = settings.vane;
  //{"<<", "<", "|", ">", ">>", "<>", "SWING"}
  doc[MA_CONST_AC_SETTING_H_SWING] = settings.wideVane;
   //Current room temperature
  doc[MA_CONST_AC_STATUS_ROOM_TEMP] = 
    ((ma_HeadPumpActualStatus.roomTemperature != NULL && 
      ma_HeadPumpActualStatus.roomTemperature > 0) ? 
        String(ma_HeadPumpActualStatus.roomTemperature).substring(0,2) : "?");
   // If true, the air conditioner is operating to reach the desired temperature
  doc[MA_CONST_AC_STATUS_OPERATING] = 
    ((ma_HeadPumpActualStatus.operating != NULL) ? 
        String(ma_HeadPumpActualStatus.operating ? "Y" : "N"): "?");
   //Compressor Frequency
  doc[MA_CONST_AC_STATUS_COMP_FREQ] = 
    ((ma_HeadPumpActualStatus.compressorFrequency != NULL) ? 
        String(ma_HeadPumpActualStatus.compressorFrequency): "?");

  //Serialize the JSON document to a String
  String airconSettingsAsJSON;
  serializeJsonPretty(doc, airconSettingsAsJSON);
  //Send back to the client as JSON
  request->send(200, "application/json", airconSettingsAsJSON); 
}

///*****************************************************************************
///Function name: handleRoot
///Description:   Handles root requests received by the webserver.
///               Creates a website providing aircon status details and the last
///               written console output which is usefull for debugging as well.
///               Also contains a ESP32 reboot link.
///Return type:   void
///Parameters:    AsyncWebServerRequest *request -> Webserver request
///*****************************************************************************
void handleRoot(AsyncWebServerRequest *request) 
{
  //Create a response object
  AsyncResponseStream *response = request->beginResponseStream("text/html");

  //Indicate that the root page was called
  queueDisplayMessage(1, 0, "Webserver:");
  queueDisplayMessage(2, 1000, "You called the root page.", 
                               MA_DEF_LED_GREEN_PIN);

  //Create the website content
  String s =  "<!DOCTYPE html>\
  <html>\
  <head>\
  <style>\
  table, th, td {\
  border: 1px solid black;\
  border-collapse: collapse;\
  }\
  th, td {\
  padding: 15px;\
  }\
  </style>\
  <body>\
  <center>\
  <h1>Mitsubishi Air Conditioner Control</h1><br>\
  <h2>";
  s = s + "Hostname: ";
  s = s + MA_DEF_WIFI_HOSTNAME;
  s = s + "<br>";
  s = s + "Current time: ";
  s = s + getCurrentTimeAsString();
  s = s + "<br>";
  s = s + "Startup time: ";
  s = s + ma_StartupTimeString;
  s = s + "<br>";
  s = s + "<br>";
  s = s + "<a href=\"Reboot\""; 
  s = s + ">Click here to reboot your ESP32</a>"; 
  s = s + "</h2><br><br>";

  if(ma_HeadPump.isConnected())
  {
    s = s + "<table>\
    <tr>\
    <th><b>KEY</b></th>\
    <th><b>VALUE</b></th>\
    </tr>";

    s = s + "<tr><th>" + MA_CONST_AC_SETTING_POWER    + "</th><th>" +
    (ma_HeadPumpActualSettings.power != NULL ? ma_HeadPumpActualSettings.power : 
      "?") + "</th></tr>";
    s = s + "<tr><th>" + MA_CONST_AC_SETTING_MODE     + "</th><th>" + 
    (ma_HeadPumpActualSettings.mode != NULL ? ma_HeadPumpActualSettings.mode : 
      "?")  + "</th></tr>";
    s = s + "<tr><th>" + MA_CONST_AC_SETTING_TEMP     + "</th><th>" + 
    ((ma_HeadPumpActualSettings.temperature != NULL && 
      ma_HeadPumpActualSettings.temperature > 0) ? 
      String(
          (
            MA_DEF_AC_USE_CELCIUS ?
            ma_HeadPumpActualSettings.temperature :
            ma_HeadPump.CelsiusToFahrenheit(
              ma_HeadPumpActualSettings.temperature)
           )
        ).substring(0, 2) :
      "?"
      ) 
    + (MA_DEF_AC_USE_CELCIUS ? "C" : "F")  
    + "</th></tr>";
    s = s + "<tr><th>" + MA_CONST_AC_SETTING_FAN      + "</th><th>" +
    (ma_HeadPumpActualSettings.fan != NULL ? ma_HeadPumpActualSettings.fan :
      "?") + "</th></tr>";
    s = s + "<tr><th>" + MA_CONST_AC_SETTING_V_SWING  + "</th><th>" + 
    (ma_HeadPumpActualSettings.vane != NULL ? ma_HeadPumpActualSettings.vane :
      "?") + "</th></tr>";
    s = s + "<tr><th>" + MA_CONST_AC_SETTING_H_SWING  + "</th><th>" + 
    (ma_HeadPumpActualSettings.wideVane != NULL ? 
      ma_HeadPumpActualSettings.wideVane : 
      "?") + "</th></tr>";
    s = s + "<tr><th>" + MA_CONST_AC_STATUS_ROOM_TEMP + "</th><th>" + 
      ((ma_HeadPumpActualStatus.roomTemperature != NULL && 
        ma_HeadPumpActualStatus.roomTemperature > 0) ? 
        String(
            (
              MA_DEF_AC_USE_CELCIUS ?
              ma_HeadPumpActualStatus.roomTemperature :
              ma_HeadPump.CelsiusToFahrenheit(
                ma_HeadPumpActualStatus.roomTemperature)
             )
          ).substring(0, 2) :
        "?"
        ) 
    + (MA_DEF_AC_USE_CELCIUS ? "C" : "F") 
    + "</th></tr>";
    s = s + "<tr><th>" + MA_CONST_AC_STATUS_OPERATING + "</th><th>" + 
      ((ma_HeadPumpActualStatus.operating != NULL) ? 
        String(ma_HeadPumpActualStatus.operating ? "Y" : "N") :
        "?"
        ) 
    + "</th></tr>";
    s = s + "<tr><th>" + MA_CONST_AC_STATUS_COMP_FREQ + "</th><th>" + 
      ((ma_HeadPumpActualStatus.compressorFrequency != NULL) ? 
        String(ma_HeadPumpActualStatus.compressorFrequency) :
        "?"
        ) 
    + "</th></tr>";
    s = s + "</table>";
  }
  else if  (ma_HeadPumpConnectedOnStartup)
  {
    s = s + "<h3>Air conditioner is currently not connected. ";
    s = s + "This might be only temporary.</h3>";
  }
  else
  {
    s = s + "<h3>Air conditioner was never connected. ";
    s = s + "Please reboot your ESP32.</h3>";
  }

  //Get the latest serial output from the buffer
  s = s + "<br><br>\
  Latest serial output:\
  <br>\
  --------------------------\
  <br>";
  
  while (!ma_SerialOutputBuffer.isEmpty())
  {
    String processMessage = ma_SerialOutputBuffer.shift();
    s = s + processMessage + "<br>";
  }
  s = s + "</center>\
  </body>\
  </html>";

  //Fill the response object
  response->addHeader("Server","Mitsubishi Aircon Webserver");
  response->print(s);
  //Send the response at last
  request->send(response);
}

///*****************************************************************************
///Function name: handleACChanges
///Description:   Handles air conditioner change requests received by the 
///               webserver. These change settings requests are provided in
///               JSON format.
///Return type:   void
///Parameters:    AsyncWebServerRequest *request -> Webserver request
///               JsonVariant &json -> JSON request body
///*****************************************************************************
void handleACChanges(AsyncWebServerRequest *request, JsonVariant &json) 
{
  //Indicate that the handleACChanges page was called
  queueDisplayMessage(1, 0, "Aircon command received.");
  queueDisplayMessage(2, 1000, "Processing", MA_DEF_LED_GREEN_PIN);

  //Check if a JSON object was received
  if ( json == NULL ) 
  {
    request->send(501, "text/html", "Command empty."); //Send web page
    queueDisplayMessage(2, 3000, "Command empty.", MA_DEF_LED_RED_PIN);
    return;
  }

  //Check if the aircon is connected
  bool checkOK = true;
  if (!ma_HeadPump.isConnected()) 
  {
    queueDisplayMessage(1, 0, "Aircon not connected.");

    if (ma_HeadPumpConnectedOnStartup)
    {
      queueDisplayMessage(2, 1000, "Auto re-connect.", MA_DEF_LED_RED_PIN);
    }
    else
    {
      queueDisplayMessage(2, 2000, "Please reboot.", MA_DEF_LED_RED_PIN);
      checkOK = false;
    }
  }

  if (checkOK)
  {
    ma_HeadPumpUpdatedSettings = ma_HeadPumpActualSettings;
   
    DynamicJsonDocument doc(1024);
    JsonObject documentRoot = json.as<JsonObject>();

    queueDisplayMessage(1, 0, "Aircon command processing");

    //Parse the changes requested and prepare the new settings
    for (JsonPair p : documentRoot) {
      String key = p.key().c_str();

      if (key.equals(MA_CONST_AC_SETTING_POWER))
      {
        ma_HeadPumpUpdatedSettings.power = p.value();
      }
      else if (key.equals(MA_CONST_AC_SETTING_MODE))
      {
        ma_HeadPumpUpdatedSettings.mode = p.value();
      }
      else if (key.equals(MA_CONST_AC_SETTING_TEMP))
      {
        ma_HeadPumpUpdatedSettings.temperature = atof(p.value());
      }
      else if (key.equals(MA_CONST_AC_SETTING_FAN))
      {
        ma_HeadPumpUpdatedSettings.fan = p.value();
      }
      else if (key.equals(MA_CONST_AC_SETTING_V_SWING))
      {
        ma_HeadPumpUpdatedSettings.vane = p.value();
      }
      else if (key.equals(MA_CONST_AC_SETTING_H_SWING))
      {
        ma_HeadPumpUpdatedSettings.wideVane = p.value();
      }
      else 
      {
        request->send(501, "text/html", (String) 
          "Setting to change is not allowed or undefined (" + key + ")."); 
        queueDisplayMessage(2, 3000, (String) 
          "Setting to change is not allowed or undefined  (" + key + ").");
        return;
      }

      //Indicate each change.
      queueDisplayMessage(2, 1000, (String) "Changing '" + key + "' to: " +
                                    p.value().as<String>(), 
                                    MA_DEF_LED_GREEN_PIN);
    }

    //We cannot update direct as the delay function inside 
    //ma_HeadPump.update() breaks the webserver.
    ma_HeadPumpUpdatedSettingsAvailable = true;

    //Send back the new settings as response.
    SendAirconSettingsJSON(request, ma_HeadPumpUpdatedSettings);
    queueDisplayMessage(1, 0, "Aircon update initiated.");
    queueDisplayMessage(2, 1000, "...", MA_DEF_LED_GREEN_PIN);
  }
  else
  {
    //In case the aircon was never connected, send back an error response.
    request->send(501, "text/html", "Aircon not connected. Please reboot."); 
    queueDisplayMessage(1, 0, "Aircon not connected.");
    setLEDState(MA_DEF_LED_RED_PIN, MA_DEF_LED_ON);
    queueDisplayMessage(2, 10000, "Please reboot.", MA_DEF_LED_RED_PIN);
  }
}

///*****************************************************************************
///Function name: handleNotFound
///Description:   Handles invalid url requests
///Return type:   void
///Parameters:    AsyncWebServerRequest *request -> Webserver request
///*****************************************************************************
void handleNotFound(AsyncWebServerRequest *request) 
{
  //Reply back the usual 404 error
  request->send(404, "text/plain", "URI not found");
}

///*****************************************************************************
///Function name: keepDisplayUpdated
///Description:   Called as a Task to process all DisplayMessage in the related
///               ring buffer
///Restriction:   Since this is a Task function use also Task specific
///               functions, e.g. vTaskDelay() instead of delay()
///Return type:   void
///Parameters:    void * parameter -> Normally NULL
///*****************************************************************************
void keepDisplayUpdated(void * parameter)
{
  //Define local variables
  Ticker ledTicker;
  int state = 0;

  //Infinite loop
  for(;;)
  { 
    //Check if there are DisplayMessages to be processed
    if (!ma_DisplayBuffer.isEmpty())
    {
      DisplayMessage processMessage = ma_DisplayBuffer.shift();

      //Set the DisplayMessage.delayMS value to 0 as the delay 
      //itself will be performed by this function!
      int delayTime = processMessage.delayMS;
      processMessage.delayMS = 0;

      //Check if LED blinking is required and perform the blinking 
      //via a Ticker
      if (delayTime > 0 && processMessage.ledPinBlink > 0)
      {
        state = digitalRead(processMessage.ledPinBlink);
        ledTicker.attach_ms(MA_DEF_LED_BLINK_INTERVAL, toggleLEDState, 
                           processMessage.ledPinBlink);
      }

      //Display the message to the display without writing it again 
      //to the Serial Console   
      writeDisplayMessage(processMessage, false);

      //Wait if required
      if (delayTime > 0)
      {
        vTaskDelay(delayTime / portTICK_PERIOD_MS);
      }

      //Stop the blinking if required
      if (delayTime > 0 && processMessage.ledPinBlink > 0)
      {
        ledTicker.detach();
        setLEDState(processMessage.ledPinBlink, state);    
      }
    }
    else
    {
      //This is important. If there are no DisplayMessages
      //to be processed allow other tasks to run.
      taskYIELD();
    }
  }
}

///*****************************************************************************
///Function name: refreshACDetails
///Description:   Called as a Task to read and update the aircon related 
///               settings and status
///Restriction:   Since this is a Task function use also Task specific
///               functions, e.g. vTaskDelay() instead of delay()
///Return type:   void
///Parameters:    void * parameter -> Normally NULL
///*****************************************************************************
void refreshACDetails(void * parameter)
{
  //Define local variables
  bool checkOK;
  unsigned long timeSinceLastTimeUpdate = millis();  
  unsigned long currentMillis = millis();  

  //On startup just wait for the setup() to finish
  vTaskDelay(500 / portTICK_PERIOD_MS);

  //Infinite loop
  for(;;)
  { 
    //Check if the aircon is connected
    checkOK = true;
    if (!ma_HeadPump.isConnected()) 
    {
       queueDisplayMessage(1, 0, "Aircon not connected.");
  
      if (ma_HeadPumpConnectedOnStartup)
      {
        queueDisplayMessage(2, 1000, "Auto re-connect.", MA_DEF_LED_RED_PIN);
      }
      else
      {
          //In case the aircon was never connected
          setLEDState(MA_DEF_LED_RED_PIN, MA_DEF_LED_ON);
          queueDisplayMessage(2, 10000, "Please reboot.", MA_DEF_LED_RED_PIN);
          checkOK = false;
      }
    }

    if (checkOK)
    {
        //Ensure the Green LED is on
        setLEDState(MA_DEF_LED_GREEN_PIN, MA_DEF_LED_ON);

        //Check if there are possible ma_HeadPumpUpdatedSettings
        //If yes, process them
        if (ma_HeadPumpUpdatedSettingsAvailable)
        {
          //Keep the updated settings as actual settings
          ma_HeadPumpActualSettings = ma_HeadPumpUpdatedSettings;
          //Update the ma_HeadPump settings.
          ma_HeadPump.setSettings(ma_HeadPumpUpdatedSettings);
          //Try up to 3 times to update the ma_HeadPump.
          //Sometimes the first try fails.
          //Even it fails, normally the changes are reflected
          //anyway.
          bool updateOK = false;
          int updateTryCounter = 0;
          while (updateOK == false && updateTryCounter <=3)
          {
            if (updateTryCounter > 0)
            {
              queueDisplayMessage(1, 0, "Aircon update failed.");
              queueDisplayMessage(2, 500, "Try number: " + 
                                          String(updateTryCounter));
              vTaskDelay(500 / portTICK_PERIOD_MS);
            }
            updateTryCounter += 1;
            updateOK = ma_HeadPump.update();
          }

          //Reset the ma_HeadPumpUpdatedSettingsAvailable indicator
          ma_HeadPumpUpdatedSettingsAvailable = false;

          //Indicate the result of the update
          if (updateOK)
          {
            queueDisplayMessage(1, 0, "Aircon update done.");
            queueDisplayMessage(2, 2000, "All changes made.", 
                                MA_DEF_LED_GREEN_PIN);

            //Just also get the actual ma_HeadPumpActualStatus
            ma_HeadPumpActualStatus = ma_HeadPump.getStatus();
          }
          else
          {
            //Even it fails, normally the changes are reflected
            //anyway.
            queueDisplayMessage(1, 0, "Aircon update failed.");
            queueDisplayMessage(2, 2000, "Changes might not be made.",
                                MA_DEF_LED_RED_PIN);
            //Sync with the air conditioner            
            queueDisplayMessage(1, 0, "Aircon:");
            queueDisplayMessage(2, 1000, "Syncing", MA_DEF_LED_GREEN_PIN);
            ma_HeadPump.sync();  
            ma_HeadPumpActualSettings = ma_HeadPump.getSettings();
            ma_HeadPumpActualStatus = ma_HeadPump.getStatus();
          }
          //Initiate a display status update
          queueDisplayStatusUpdate();
        }
        else
        {
          //In case there was no settings update requested, just sync
          ma_HeadPump.sync();  //Sync with the air conditioner
          ma_HeadPumpActualSettings = ma_HeadPump.getSettings();
          ma_HeadPumpActualStatus = ma_HeadPump.getStatus();

          //It's enough to update the display maximum once per second
          currentMillis = millis();
          if ((currentMillis - timeSinceLastTimeUpdate) >= 1000 || 
               currentMillis < timeSinceLastTimeUpdate)
          {
              timeSinceLastTimeUpdate = millis();
              //Initiate a display status update
              queueDisplayStatusUpdate();
          }
        }
    }
    else
    {
      //In case the aircon was never connected, turn of the 
      //Green LED and end this task as it is not longer required.
      setLEDState(MA_DEF_LED_GREEN_PIN, MA_DEF_LED_OFF);
      
      if( ma_AirconTaskHandle != NULL )
      {
          //The task is going to be deleted. Set the handle to NULL.
          ma_AirconTaskHandle = NULL;
          // Delete the task. 
          vTaskDelete(NULL);
      }
    }
  }
}

///*****************************************************************************
///SETUP
///*****************************************************************************
void setup() 
{
  //Put your setup code here, to run once.

  //Initialise the Serial port
  //It is important to set the Serial0 (aka Serial) baud rate to at least 
  //115200, otherwise e.g. the display refresh will be very slow.
  //This is related to the code which also writes to the Serial Console.
  Serial.begin(115200);
  //Initialise the Serial2 port used to communicate with the air conditioner
  Serial2.begin(9600, SERIAL_8N1, MA_DEF_AC_RX_PIN, MA_DEF_AC_TX_PIN); 

  //Disable Bluetooth
  btStop();

  // Initialize the LED's
  pinMode(MA_DEF_LED_RED_PIN, OUTPUT);
  pinMode(MA_DEF_LED_YELLOW_PIN, OUTPUT);
  pinMode(MA_DEF_LED_GREEN_PIN, OUTPUT);
  setLEDState(MA_DEF_LED_RED_PIN, MA_DEF_LED_ON);
  setLEDState(MA_DEF_LED_YELLOW_PIN, MA_DEF_LED_ON);
  setLEDState(MA_DEF_LED_GREEN_PIN, MA_DEF_LED_ON);

  //Initialise the display
  if (!ma_Display.begin(SSD1306_SWITCHCAPVCC, MA_CONST_SCREEN_ADDRESS)) 
  {
    Serial.println("Display - SSD1306 allocation failed.");
    Serial.println("I died :(");
    for (;;); //Loop forever
  }
  else
  {
    //Enable display text wrapping
    ma_Display.setTextWrap(true);
    nonTaskDelay(1000);
  }

  //Change the LED state
  setLEDState(MA_DEF_LED_RED_PIN, MA_DEF_LED_OFF);
  setLEDState(MA_DEF_LED_YELLOW_PIN, MA_DEF_LED_OFF);
  setLEDState(MA_DEF_LED_GREEN_PIN, MA_DEF_LED_OFF);

  //Show initial display buffer contents on the screen;
  //the library initializes this with an Adafruit splash screen.
  //Afterwards, clear the display
  ma_Display.display();
  nonTaskDelay(2000, MA_DEF_LED_RED_PIN);
  ma_Display.clearDisplay();
  ma_Display.setTextColor(WHITE,BLACK);

  writeDisplayMessage(1, 1000, "Setup starting.", MA_DEF_LED_YELLOW_PIN);

  //Setup WiFi
  writeDisplayMessage(1, 1000, "Setup Wifi", MA_DEF_LED_YELLOW_PIN);
  WiFi.setHostname(MA_DEF_WIFI_HOSTNAME);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.persistent(true);  
  //Connect to the network
  WiFi.begin(MA_DEF_WIFI_SSID, MA_DEF_WIFI_PASSWORD); 
  writeDisplayMessage(1, 0, "Connecting to SSID:");
  writeDisplayMessage(2, 1000, MA_DEF_WIFI_SSID, MA_DEF_LED_YELLOW_PIN);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { 
    // Wait for the Wi-Fi to connect
    writeDisplayMessage(2, 1000, (String) MA_DEF_WIFI_SSID + 
                        " (" + (++i) + " sec)", MA_DEF_LED_YELLOW_PIN);
  }

  writeDisplayMessage(1, 0, "Wifi connected");
  writeDisplayMessage(2, 5000, (String) "IP: " + WiFi.localIP().toString() +
                      "\nStrength: " + WiFi.RSSI() + " dBm", 
                      MA_DEF_LED_YELLOW_PIN);

  //Setup Time
  writeDisplayMessage(1, 0, "Getting time from NTP");
  writeDisplayMessage(2, 1000, "...", MA_DEF_LED_YELLOW_PIN);
  ma_TimeClient.begin();
  setSyncProvider(getNTPTime);
  ma_StartupTimeString = getCurrentTimeAsString();  
  writeDisplayMessage(1, 0, "Current time:");
  writeDisplayMessage(2, 1000, ma_StartupTimeString , MA_DEF_LED_YELLOW_PIN);

  //Connect to air conditioner 
  writeDisplayMessage(1, 0, "Connecting to Aircon");
  writeDisplayMessage(2, 1000, "...", MA_DEF_LED_YELLOW_PIN);
  ma_HeadPump.connect(&Serial2);
  if (ma_HeadPump.isConnected()) 
  {
      //Enable external updates via the remote.
      ma_HeadPump.enableExternalUpdate();
      writeDisplayMessage(2, 1000, "Sync with Aircon.", MA_DEF_LED_YELLOW_PIN);
      //Sync with the air conditioner
      ma_HeadPump.sync(); 
      writeDisplayMessage(2, 1000, "Read Aircon settings.", 
                          MA_DEF_LED_YELLOW_PIN);
      //Get air conditioner settings
      ma_HeadPumpActualSettings = ma_HeadPump.getSettings(); 
      writeDisplayMessage(2, 1000, "Read Aircon status.", 
                          MA_DEF_LED_YELLOW_PIN);
      //Get air conditioner status
      ma_HeadPumpActualStatus = ma_HeadPump.getStatus();
      writeDisplayMessage(1, 0, "Aircon connected.");
      writeDisplayMessage(2, 1000, "...", MA_DEF_LED_YELLOW_PIN);
  }
  else
  {
    //Air conditioner could not be connected. Better check the wires ;)
    ma_HeadPumpConnectedOnStartup = false;
    writeDisplayMessage(1, 0, "Aircon not connected.");
    writeDisplayMessage(2, 3000, "...", MA_DEF_LED_RED_PIN);
  }

  //Setup Arduino OTA
  writeDisplayMessage(1, 0, "Setup OTA update");
  writeDisplayMessage(2, 1000, "...", MA_DEF_LED_YELLOW_PIN);
  ArduinoOTA.setHostname(MA_DEF_WIFI_HOSTNAME);
  ArduinoOTA.setPassword(MA_DEF_OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    //When OTA starts remove the air conditioner task 
    if( ma_AirconTaskHandle != NULL )
    {
        //Delete the task
        vTaskDelete(ma_AirconTaskHandle);
        ma_AirconTaskHandle = NULL;
    }
    queueDisplayMessage(1, 0, "OTA update");
    queueDisplayMessage(2, 1000, "Started", MA_DEF_LED_RED_PIN);
  });
  ArduinoOTA.onEnd([]() {
    queueDisplayMessage(1, 0, "OTA update");
    queueDisplayMessage(2, 3000, "Finished", MA_DEF_LED_RED_PIN);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    queueDisplayMessage(1, 0, "OTA update");
    queueDisplayMessage(2, 0, (String) "Progress: " + 
                        (progress / (total / 100)) + "%", MA_DEF_LED_RED_PIN);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    queueDisplayMessage(1, 0, "OTA update error");
    if (error == OTA_AUTH_ERROR) queueDisplayMessage(2, 3000, 
                                  "Auth failed", MA_DEF_LED_RED_PIN);
    else if (error == OTA_BEGIN_ERROR) queueDisplayMessage(2, 3000, 
                                  "Begin failed", MA_DEF_LED_RED_PIN);
    else if (error == OTA_CONNECT_ERROR) queueDisplayMessage(2, 3000, 
                                  "Connect failed", MA_DEF_LED_RED_PIN);
    else if (error == OTA_RECEIVE_ERROR) queueDisplayMessage(2, 3000, 
                                  "Receive failed", MA_DEF_LED_RED_PIN);
    else if (error == OTA_END_ERROR) queueDisplayMessage(2, 3000, 
                                  "End failed", MA_DEF_LED_RED_PIN);
  });
  ArduinoOTA.begin();

  //Setup the webserver
  writeDisplayMessage(1, 0, "Setup webserver");
  writeDisplayMessage(2, 1000, "Port: " + (String) MA_DEF_WEBSERVER_PORT, 
                      MA_DEF_LED_YELLOW_PIN);

  ma_Webserver.on("/", [](AsyncWebServerRequest * request) 
  {
    handleRoot(request);
  });       //Which routine to handle at root location. This is display page
  ma_Webserver.on("/Reboot", [](AsyncWebServerRequest * request) 
  {
    request->send(200, "text/html", "Reboot initiated."); //Send web page
    ma_InitiateReboot = true;
  });       
  ma_Webserver.on("/GetAirconSettings", [](AsyncWebServerRequest * request) 
  {
    SendAirconSettingsJSON(request);
  });       
  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler(
    "/ChangeAirconSettings", [](AsyncWebServerRequest *request, 
    JsonVariant &json) 
    {
      handleACChanges(request, json);
    }
  );
  ma_Webserver.addHandler(handler);
  ma_Webserver.onNotFound([](AsyncWebServerRequest * request) 
  {
    handleNotFound(request);
  });

  //Start the webserver
  ma_Webserver.begin();              
  writeDisplayMessage(1, 0, "Webserver started");
  writeDisplayMessage(2, 1000, "...", MA_DEF_LED_YELLOW_PIN);

  //Start the two main tasks 
  //Both must be running at the same priority!
  writeDisplayMessage(1, 0, "Starting tasks");
  writeDisplayMessage(2, 1000, "...", MA_DEF_LED_YELLOW_PIN);

  //It is important that the display related task is running
  //on the ARDUINO_RUNNING_CORE, otherwise the display output
  //is garbage.
  xTaskCreatePinnedToCore(
    keepDisplayUpdated,       // Function that should be called
    "Display Update",         // Name of the task (for debugging)
    3000,                     // Stack size (bytes)
    NULL,                     // Parameter to pass
    1,                        // Task priority
    &ma_DisplayTaskHandle,    // Task handle
    ARDUINO_RUNNING_CORE      // Task runnig core
  );

  //The air conditioner related task runs on the second core to
  //balance the load.
  xTaskCreatePinnedToCore(
    refreshACDetails,         // Function that should be called
    "AC Update",              // Name of the task (for debugging)
    12000,                    // Stack size (bytes)
    NULL,                     // Parameter to pass
    1,                        // Task priority
    &ma_AirconTaskHandle,     // Task handle 
    ARDUINO_NONRUNNING_CORE   // Task runnig core
  );

  queueDisplayMessage(1, 0, "Setup done");
  queueDisplayMessage(2, 2000, "----------");
}

///*****************************************************************************
/// LOOP
///*****************************************************************************
void loop() {
  //Put your main code here, to run repeatedly.

  //Check if WiFi is still up.
  //Sometimes after e.g. a router reboot the ESP can't reconnect.
  if(WiFi.status() == WL_CONNECTED)
  {
      if (ma_WiFiReconnectCount > 0)
      {
        ma_WiFiReconnectCount = 0;
        queueDisplayMessage(1, 0, "Wifi reconnected");
        queueDisplayMessage(2, 5000, (String) "IP: " + 
                        WiFi.localIP().toString() +
                        "\nStrength: " + WiFi.RSSI() + " dBm", 
                        MA_DEF_LED_YELLOW_PIN);
      }
  }
  else
  {
      if (ma_WiFiReconnectCount < MA_DEF_WIFI_MAX_RECONNECT_COUNT)
      {
        queueDisplayMessage(1, 0, "Wifi disconnected");
        queueDisplayMessage(2, 5000, (String) "Reconnection try no. " + 
                        ma_WiFiReconnectCount + " of " + 
                        MA_DEF_WIFI_MAX_RECONNECT_COUNT, 
                        MA_DEF_LED_YELLOW_PIN);
        WiFi.reconnect();
        ma_WiFiReconnectCount++;
        nonTaskDelay(5000);
      }
      else
      {
        ma_InitiateReboot = true;
      }
  }

  //Check if a reboot was requested
  if (ma_InitiateReboot)
  {
    //End all tasks if a reboot was requested.
    if(ma_DisplayTaskHandle != NULL)
    {
      vTaskDelete(ma_DisplayTaskHandle);
      ma_DisplayTaskHandle = NULL;
    }
    if(ma_AirconTaskHandle != NULL)
    {
      vTaskDelete(ma_AirconTaskHandle);
      ma_AirconTaskHandle = NULL;
    }
    writeDisplayMessage(1, 0, "Reboot");
    writeDisplayMessage(2, 5000, "Reboot initiated. Restarting soon.", 
                        MA_DEF_LED_RED_PIN);
    //Reboot the ESP32
    ESP.restart();
  }

  //Process OTA requests, if any
  ArduinoOTA.handle();
  
}
