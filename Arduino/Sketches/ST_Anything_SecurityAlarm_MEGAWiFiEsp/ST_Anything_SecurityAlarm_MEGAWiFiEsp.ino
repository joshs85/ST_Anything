//******************************************************************************************
//  File: ST_Anything_Multiples_MEGAWiFiEsp.ino
//  Authors: Dan G Ogorchock & Daniel J Ogorchock (Father and Son)
//
//  Summary:  This Arduino Sketch, along with the ST_Anything library and the revised SmartThings 
//            library, demonstrates the ability of one Arduino MEGA + ESP-01 board to 
//            implement a multi input/output custom device for integration into SmartThings.
//            The ST_Anything library takes care of all of the work to schedule device updates
//            as well as all communications with the ESP-01 board.
//
//            ST_Anything_Multiples implements the following ST Capabilities in multiples of 2 as a demo of what is possible with a single Arduino
//              - 2 x Door Control devices (used typically for Garage Doors - input pin (contact sensor) and output pin (relay switch)
//              - 2 x Contact Sensor devices (used to monitor magnetic door sensors)
//              - 2 x Switch devices (used to turn on a digital output (e.g. LED, relay, etc...)
//              - 2 x Water Sensor devices (using an analog input pin to measure voltage from a water detector baord)
//              - 2 x Illuminance Measurement devices (using a photoresitor attached to ananlog input)
//              - 2 x Voltage Measurement devices (using a photoresitor attached to ananlog input)
//              - 2 x Smoke Detector devices (using simple digital input)
//              - 2 x Carbon Monoxide Detector devices (using simple digital input)
//              - 2 x Motion devices (used to detect motion)
//              - 2 x Temperature Measurement devices (Temperature from DHT22 device)
//              - 2 x Humidity Measurement devices (Humidity from DHT22 device)
//              - 2 x Relay Switch devices (used to turn on a digital output for a set number of cycles And On/Off times (e.g.relay, etc...))
//              - 2 x Button devices (sends "pushed" if held for less than 1 second, else sends "held"
//              - 2 x Alarm devices - 1 siren only, 1 siren and strobe (using simple digital outputs)
//
//            This example requires the use of an Arduino MEGA2560 since the ESP-01 WiFi board
//            needs a high-speed UART for communications.  This example assumes the ESP-01 is
//            connected to the MEGA's "Serial1" UART on pins 18 & 19.
//
//    
//  Change History:
//
//    Date        Who            What
//    ----        ---            ----
//    2015-01-03  Dan & Daniel   Original Creation
//    2017-02-12  Dan Ogorchock  Revised to use the new SMartThings v2.0 library
//    2017-02-20  Dan Ogorchock  Revised to use the ESP-01 board
//    2017-04-16  Dan Ogorchock  New sketch to demonstrate multiple SmartThings Capabilties of each type
//    2017-04-22  Dan Ogorchock  Added Voltage, Carbon Monoxide, and Alarm with Strobe
//
//******************************************************************************************
//******************************************************************************************
// SmartThings Library for Arduino + ESP-01 board combination.
//******************************************************************************************
#include <SmartThingsWiFiEsp.h>    //Library to provide API to the SmartThings ESP-01 WiFi board

//******************************************************************************************
// ST_Anything Library 
//******************************************************************************************
#include <Constants.h>       //Constants.h is designed to be modified by the end user to adjust behavior of the ST_Anything library
#include <Device.h>          //Generic Device Class, inherited by Sensor and Executor classes
#include <Sensor.h>          //Generic Sensor Class, typically provides data to ST Cloud (e.g. Temperature, Motion, etc...)
#include <Executor.h>        //Generic Executor Class, typically receives data from ST Cloud (e.g. Switch)
#include <InterruptSensor.h> //Generic Interrupt "Sensor" Class, waits for change of state on digital input 
#include <PollingSensor.h>   //Generic Polling "Sensor" Class, polls Arduino pins periodically
#include <Everything.h>      //Master Brain of ST_Anything library that ties everything together and performs ST Shield communications

//#include <PS_Illuminance.h>  //Implements a Polling Sensor (PS) to measure light levels via a photo resistor on an analog input pin 
//#include <PS_Voltage.h>      //Implements a Polling Sensor (PS) to measure voltage on an analog input pin 
//#include <PS_TemperatureHumidity.h>  //Implements a Polling Sensor (PS) to measure Temperature and Humidity via DHT library
//#include <PS_Water.h>        //Implements a Polling Sensor (PS) to measure presence of water (i.e. leak detector) on an analog input pin 
#include <IS_Motion.h>       //Implements an Interrupt Sensor (IS) to detect motion via a PIR sensor on a digital input pin
#include <IS_Contact.h>      //Implements an Interrupt Sensor (IS) to monitor the status of a digital input pin
//#include <IS_Smoke.h>        //Implements an Interrupt Sensor (IS) to monitor the status of a digital input pin
//#include <IS_CarbonMonoxide.h> //Implements an Interrupt Sensor (IS) to monitor the status of a digital input pin
//#include <IS_DoorControl.h>  //Implements an Interrupt Sensor (IS) and Executor to monitor the status of a digital input pin and control a digital output pin
//#include <IS_Button.h>       //Implements an Interrupt Sensor (IS) to monitor the status of a digital input pin for button presses
//#include <EX_Switch.h>       //Implements an Executor (EX) via a digital output to a relay
//#include <EX_Alarm.h>        //Implements Executor (EX)as an Alarm capability with Siren and Strobe via digital outputs to relays
//#include <S_TimedRelay.h>    //Implements a Sensor to control a digital output pin with timing/cycle repeat capabilities

//**********************************************************************************************************
//Define which Arduino Pins will be used for each device
//  Notes: Arduino communicates with the ESP-01 using the "Serial1" Hardware Serial UART. 
//         This is on digital pins 18 & 19 on the Mega. 
//**********************************************************************************************************
//"RESERVED" pins for ESP-01 - best to avoid
#define PIN_18_RESERVED          18  //reserved ESP-01 on MEGA
#define PIN_19_RESERVED          19  //reserved ESP-01 on MEGA

#define PIN_MOTION_1              24  //SmartThings Capability "Motion Sensor"
#define PIN_MOTION_2              25  //SmartThings Capability "Motion Sensor"
#define PIN_CONTACT_1             26  //SmartThings Capability "Contact Sensor"
#define PIN_CONTACT_2             27  //SmartThings Capability "Contact Sensor"


//******************************************************************************************
//WiFiEsp Information
//****************************************************************************************** 
String str_ssid     = "Atheist TeslaNet";                            //  <---You must edit this line!
String str_password = "Powerlocksarebetterthanpadlocks!";                    //  <---You must edit this line!
IPAddress ip(192, 168, 1, 228);       // Device IP Address       //  <---You must edit this line!
const unsigned int serverPort = 8090; // port to run the http server on

// Smartthings hub information
IPAddress hubIp(192,168,1,149);       // smartthings hub ip      //  <---You must edit this line!
const unsigned int hubPort = 39500;   // smartthings hub port

//******************************************************************************************
//st::Everything::callOnMsgSend() optional callback routine.  This is a sniffer to monitor 
//    data being sent to ST.  This allows a user to act on data changes locally within the 
//    Arduino sktech.
//******************************************************************************************
void callback(const String &msg)
{
  //Uncomment if it weould be desirable to using this function
  //Serial.print(F("ST_Anything_Miltiples Callback: Sniffed data = "));
  //Serial.println(msg);
  
  //TODO:  Add local logic here to take action when a device's value/state is changed
  
  //Masquerade as the ThingShield to send data to the Arduino, as if from the ST Cloud (uncomment and edit following line(s) as you see fit)
  //st::receiveSmartString("Put your command here!");  //use same strings that the Device Handler would send
}

//******************************************************************************************
//Arduino Setup() routine
//******************************************************************************************
void setup()
{
  //******************************************************************************************
  //Declare each Device that is attached to the Arduino
  //  Notes: - For each device, there is typically a corresponding "tile" defined in your 
  //           SmartThings Device Hanlder Groovy code, except when using new COMPOSITE Device Handler
  //         - For details on each device's constructor arguments below, please refer to the 
  //           corresponding header (.h) and program (.cpp) files.
  //         - The name assigned to each device (1st argument below) must match the Groovy
  //           Device Handler names.  (Note: "temphumid" below is the exception to this rule
  //           as the DHT sensors produce both "temperature" and "humidity".  Data from that
  //           particular sensor is sent to the ST Hub in two separate updates, one for 
  //           "temperature" and one for "humidity")
  //         - The new Composite Device Handler is comprised of a Parent DH and various Child
  //           DH's.  The names used below MUST not be changed for the Automatic Creation of
  //           child devices to work properly.  Simply increment the number by +1 for each duplicate
  //           device (e.g. contact1, contact2, contact3, etc...)  You can rename the Child Devices
  //           to match your specific use case in the ST Phone Application.
  //******************************************************************************************
//Polling Sensors 
  //Interrupt Sensors 
  static st::IS_Motion              sensor9(F("motion1"), PIN_MOTION_1, HIGH, false, 500);
  static st::IS_Motion              sensor10(F("motion2"), PIN_MOTION_2, HIGH, false, 500);
  static st::IS_Contact             sensor11(F("contact1"), PIN_CONTACT_1, LOW, true, 500);
  static st::IS_Contact             sensor12(F("contact2"), PIN_CONTACT_2, LOW, true, 500);
   
  //*****************************************************************************
  //  Configure debug print output from each main class 
  //*****************************************************************************
  st::Everything::debug=true;
  st::Executor::debug=true;
  st::Device::debug=true;
  st::PollingSensor::debug=true;
  st::InterruptSensor::debug=true;

  //*****************************************************************************
  //Initialize the "Everything" Class
  //*****************************************************************************
  
  //Initialize the optional local callback routine (safe to comment out if not desired)
  st::Everything::callOnMsgSend = callback;
  
  //Create the SmartThings WiFiEsp (ESP-01) Communications Object
    //STATIC IP Assignment - Recommended
    st::Everything::SmartThing = new st::SmartThingsWiFiEsp(&Serial, str_ssid, str_password, ip, serverPort, hubIp, hubPort, st::receiveSmartString);
 
    //DHCP IP Assigment - Must set your router's DHCP server to provice a static IP address for this device's MAC address
    //st::Everything::SmartThing = new st::SmartThingsWiFiEsp(&Serial1, str_ssid, str_password, serverPort, hubIp, hubPort, st::receiveSmartString);

  //Initialize the Serial1 baudrate to match your ESP-01's baud rate (e.g. 9600, 57600, 115200)
  Serial.begin(115200); 
  
  //Run the Everything class' init() routine which establishes Ethernet communications with the SmartThings Hub
  st::Everything::init();
  
  //*****************************************************************************
  //Add each sensor to the "Everything" Class
  //*****************************************************************************
  st::Everything::addSensor(&sensor9); 
  st::Everything::addSensor(&sensor10); 
  st::Everything::addSensor(&sensor11);
  st::Everything::addSensor(&sensor12);
  
  //*****************************************************************************
  //Add each executor to the "Everything" Class
  //*****************************************************************************

  
  //*****************************************************************************
  //Initialize each of the devices which were added to the Everything Class
  //*****************************************************************************
  st::Everything::initDevices();
  
}

//******************************************************************************************
//Arduino Loop() routine
//******************************************************************************************
void loop()
{
  //*****************************************************************************
  //Execute the Everything run method which takes care of "Everything"
  //*****************************************************************************
  st::Everything::run();
}
