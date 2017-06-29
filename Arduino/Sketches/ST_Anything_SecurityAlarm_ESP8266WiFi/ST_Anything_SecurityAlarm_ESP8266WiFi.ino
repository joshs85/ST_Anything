//******************************************************************************************
//  File: ST_Anything_AlarmPanel_ESP8266WiFi.ino
//  Authors: Dan G Ogorchock & Daniel J Ogorchock (Father and Son)
//
//  Summary:  This Arduino Sketch, along with the ST_Anything library and the revised SmartThings 
//            library, demonstrates the ability of one NodeMCU ESP8266 to 
//            implement a multi input/output custom device for integration into SmartThings.
//            The ST_Anything library takes care of all of the work to schedule device updates
//            as well as all communications with the NodeMCU ESP8266's WiFi.
//
//            ST_Anything_AlarmPanel implements the following ST Capabilities as a demo of replacing an alarm panel with a single NodeMCU ESP8266
//              - 7 x Contact Sensor devices (used to monitor magnetic door sensors)
//              - 1 x Motion device (using simple digital input)
//              - 1 x Smoke Detector devices (using simple digital input)
//              - 1 x Alarm (Siren only) device (using a simple digital output attached to a relay)
//    
//  Change History:
//
//    Date        Who            What
//    ----        ---            ----
//    2017-04-26  Dan Ogorchock  Original Creation

//******************************************************************************************
//******************************************************************************************
// SmartThings Library for ESP8266WiFi
//******************************************************************************************
#include <SmartThingsESP32WiFi.h>
#include <esp_task_wdt.h>

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
#include <EX_Alarm.h>        //Implements Executor (EX)as an Alarm capability with Siren and Strobe via digital outputs to relays
//#include <EX_Blind.h>        //Implements Executor (EX)as an Blind capability
//#include <S_TimedRelay.h>    //Implements a Sensor to control a digital output pin with timing/cycle repeat capabilities

//*************************************************************************************************
//NodeMCU v1.0 ESP32 Pin Definitions (makes it much easier as these match the board markings)
//*************************************************************************************************
#define LED_BUILTIN 5
#define BUTTON_PIN = 0;

//******************************************************************************************
//Define which Arduino Pins will be used for each device
//ESP32 PINOUT: https://cdn.sparkfun.com/assets/learn_tutorials/5/0/7/esp32-thing-graphical-datasheet-v02.png
//WatchDog Timer: http://www.switchdoc.com/dual-watchdog-timer/
//******************************************************************************************

#define PIN_CONTACT_1             21    //Dining Room Window Left
#define PIN_CONTACT_2             1     //UnUsed, TX
#define PIN_CONTACT_3             3     //UnUsed, RX
#define PIN_CONTACT_4             22    //Dining Room Window Right
#define PIN_CONTACT_5             19    //Sun Room East Right WIndow
#define PIN_CONTACT_6             23    //Basement Half Window
#define PIN_CONTACT_7             18    //Sun Room East Middle Window
#define PIN_CONTACT_8             5     //UnUsed
#define PIN_CONTACT_9             15    //Sun Room East Left Window
#define PIN_CONTACT_10             2    //UnUsed
#define PIN_CONTACT_11             0    //UnUsed
#define PIN_CONTACT_12             4    //UnUsed
#define PIN_CONTACT_13             17   //UnUsed
#define PIN_CONTACT_14             16   //Siren
#define PIN_CONTACT_15             36   //Basement Window Right, No internal pullup
#define PIN_CONTACT_16             37   //Basement Window Left, No internal pullup
#define PIN_CONTACT_17             38   //Basement Door, No internal pullup
#define PIN_CONTACT_18             39   //Sun Room Door, No internal pullup
#define PIN_CONTACT_19             32   //Office Window Right
#define PIN_CONTACT_20             33   //Office Window Left
#define PIN_CONTACT_21             34   //Kitchen Window, No internal pullup
#define PIN_CONTACT_22             35   //Sun Room N Right Window, No internal pullup
#define PIN_CONTACT_23             25   //Living Room R Window
#define PIN_CONTACT_24             26   //Living Room 2 Left Window
#define PIN_CONTACT_25             27   //Living Room L Window
#define PIN_CONTACT_26             14   //Sun Room North Left Windows
#define PIN_CONTACT_27             12   //Guest Bedroom Windows
#define PIN_CONTACT_28             13   //Master Bedroom Windows

//Renamed and placed in unused slots so that numbers (contact##) are contiguous.
#define PIN_MAPPED_CONTACT_2        PIN_CONTACT_21
#define PIN_MAPPED_CONTACT_3        PIN_CONTACT_22
#define PIN_MAPPED_CONTACT_8        PIN_CONTACT_23
#define PIN_MAPPED_CONTACT_10       PIN_CONTACT_24
#define PIN_MAPPED_CONTACT_11       PIN_CONTACT_25
#define PIN_MAPPED_CONTACT_12       PIN_CONTACT_26
#define PIN_MAPPED_CONTACT_13       PIN_CONTACT_27
#define PIN_MAPPED_CONTACT_14       PIN_CONTACT_28

#define PIN_ALARM_1                PIN_CONTACT_14   //Siren

//******************************************************************************************
//ESP32 WiFi Information
//******************************************************************************************
String str_ssid     = "Atheist TeslaNet";                       //  <---You must edit this line!
String str_password = "Powerlocksarebetterthanpadlocks!";       //  <---You must edit this line!
//IPAddress ip(192, 168, 3, 236);       //Device IP Address       //  <---You must edit this line!
//IPAddress gateway(192, 168, 3, 1);    //Router gateway          //  <---You must edit this line!
//IPAddress subnet(255, 255, 255, 0);   //LAN subnet mask         //  <---You must edit this line!
//IPAddress dnsserver(192, 168, 3, 1);  //DNS server              //  <---You must edit this line!
const unsigned int serverPort = 8090; // port to run the http server on

// Smartthings Hub Information
IPAddress hubIp(192, 168, 3, 174);    // smartthings hub ip     //  <---You must edit this line!
const unsigned int hubPort = 39500;   // smartthings hub port

//******************************************************************************************
//st::Everything::callOnMsgSend() optional callback routine.  This is a sniffer to monitor 
//    data being sent to ST.  This allows a user to act on data changes locally within the 
//    Arduino sktech.
//******************************************************************************************
void callback(const String &msg)
{
//  Serial.print(F("ST_Anything Callback: Sniffed data = "));
//  Serial.println(msg);
  
  //TODO:  Add local logic here to take action when a device's value/state is changed
  
  //Masquerade as the ThingShield to send data to the Arduino, as if from the ST Cloud (uncomment and edit following line)
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
static st::IS_Contact             sensor1(F("contact1"), PIN_CONTACT_1, LOW, true, 1000);

//static st::IS_Contact             sensor2(F("contact2"), PIN_CONTACT_2, LOW, true, 1000);
//static st::IS_Contact             sensor3(F("contact3"), PIN_CONTACT_3, LOW, true, 1000);
static st::IS_Contact             sensor2(F("contact2"), PIN_MAPPED_CONTACT_2, LOW, false, 1000);
static st::IS_Contact             sensor3(F("contact3"), PIN_MAPPED_CONTACT_3, LOW, false, 1000);

static st::IS_Contact             sensor4(F("contact4"), PIN_CONTACT_4, LOW, true, 1000);
static st::IS_Contact             sensor5(F("contact5"), PIN_CONTACT_5, LOW, true, 1000);
static st::IS_Contact             sensor6(F("contact6"), PIN_CONTACT_6, LOW, true, 1000);
static st::IS_Contact             sensor7(F("contact7"), PIN_CONTACT_7, LOW, true, 1000);

//static st::IS_Contact             sensor8(F("contact8"), PIN_CONTACT_8, LOW, true, 1000);
static st::IS_Contact             sensor8(F("contact8"), PIN_MAPPED_CONTACT_8, LOW, true, 1000);

static st::IS_Contact             sensor9(F("contact9"), PIN_CONTACT_9, LOW, true, 1000);
//static st::IS_Contact             sensor10(F("contact10"), PIN_CONTACT_10, LOW, true, 1000);
//static st::IS_Contact             sensor11(F("contact11"), PIN_CONTACT_11, LOW, true, 1000);
//static st::IS_Contact             sensor12(F("contact12"), PIN_CONTACT_12, LOW, true, 1000);
//static st::IS_Contact             sensor13(F("contact13"), PIN_CONTACT_13, LOW, true, 1000);
//static st::IS_Contact             sensor14(F("contact14"), PIN_CONTACT_14, LOW, true, 1000);
static st::IS_Contact             sensor10(F("contact10"), PIN_MAPPED_CONTACT_10, LOW, true, 1000);
static st::IS_Contact             sensor11(F("contact11"), PIN_MAPPED_CONTACT_11, LOW, true, 1000);
static st::IS_Contact             sensor12(F("contact12"), PIN_MAPPED_CONTACT_12, LOW, true, 1000);
static st::IS_Contact             sensor13(F("contact13"), PIN_MAPPED_CONTACT_13, LOW, true, 1000);
static st::IS_Contact             sensor14(F("contact14"), PIN_MAPPED_CONTACT_14, LOW, true, 1000);

static st::IS_Contact             sensor15(F("contact15"), PIN_CONTACT_15, LOW, false, 1000);
static st::IS_Contact             sensor16(F("contact16"), PIN_CONTACT_16, LOW, false, 1000);
static st::IS_Contact             sensor17(F("contact17"), PIN_CONTACT_17, LOW, false, 1000);
static st::IS_Contact             sensor18(F("contact18"), PIN_CONTACT_18, LOW, false, 1000);
static st::IS_Contact             sensor19(F("contact19"), PIN_CONTACT_19, LOW, true, 1000);
static st::IS_Contact             sensor20(F("contact20"), PIN_CONTACT_20, LOW, true, 1000);

//static st::IS_Contact             sensor21(F("contact21"), PIN_CONTACT_21, LOW, false, 1000);
//static st::IS_Contact             sensor22(F("contact22"), PIN_CONTACT_22, LOW, false, 1000);
//static st::IS_Contact             sensor23(F("contact23"), PIN_CONTACT_23, LOW, true, 1000);
//static st::IS_Contact             sensor24(F("contact24"), PIN_CONTACT_24, LOW, true, 1000);
//static st::IS_Contact             sensor25(F("contact25"), PIN_CONTACT_25, LOW, true, 1000);
//static st::IS_Contact             sensor26(F("contact26"), PIN_CONTACT_26, LOW, true, 1000);
//static st::IS_Contact             sensor27(F("contact27"), PIN_CONTACT_27, LOW, true, 1000);
//static st::IS_Contact             sensor28(F("contact28"), PIN_CONTACT_28, LOW, true, 1000);

//Executors
static st::EX_Alarm               executor1(F("alarm1"), PIN_ALARM_1, LOW, false);
  
  //*****************************************************************************
  //  Configure debug print output from each main class 
  //  -Note: Set these to "false" if using Hardware Serial on pins 0 & 1
  //         to prevent communication conflicts with the ST Shield communications
  //*****************************************************************************
  st::Everything::debug=false;
  st::Executor::debug=false;
  st::Device::debug=false;
  st::PollingSensor::debug=false;
  st::InterruptSensor::debug=true;

  //Start Watchdog Timer
  esp_task_wdt_init();
  //*****************************************************************************
  //Initialize the "Everything" Class
  //*****************************************************************************

  //Initialize the optional local callback routine (safe to comment out if not desired)
  st::Everything::callOnMsgSend = callback;
  
  //Create the SmartThings ESP8266WiFi Communications Object
    //STATIC IP Assignment - Recommended
    //st::Everything::SmartThing = new st::SmartThingsESP32WiFi(str_ssid, str_password, ip, gateway, subnet, dnsserver, serverPort, hubIp, hubPort, st::receiveSmartString);
 
    //DHCP IP Assigment - Must set your router's DHCP server to provice a static IP address for this device's MAC address
    st::Everything::SmartThing = new st::SmartThingsESP32WiFi(str_ssid, str_password, serverPort, hubIp, hubPort, st::receiveSmartString);

  //Run the Everything class' init() routine which establishes WiFi communications with SmartThings Hub
  st::Everything::init();
    
  //*****************************************************************************
  //Add each sensor to the "Everything" Class
  //*****************************************************************************
  st::Everything::addSensor(&sensor1);
  st::Everything::addSensor(&sensor2);
  st::Everything::addSensor(&sensor3);
  st::Everything::addSensor(&sensor4); 
  st::Everything::addSensor(&sensor5); 
  st::Everything::addSensor(&sensor6); 
  st::Everything::addSensor(&sensor7); 
  st::Everything::addSensor(&sensor8);
  st::Everything::addSensor(&sensor9); 
  st::Everything::addSensor(&sensor10);

  st::Everything::addSensor(&sensor11);
  st::Everything::addSensor(&sensor12);
  st::Everything::addSensor(&sensor13);
  st::Everything::addSensor(&sensor14); 
  st::Everything::addSensor(&sensor15); 
  st::Everything::addSensor(&sensor16); 
  st::Everything::addSensor(&sensor17); 
  st::Everything::addSensor(&sensor18);
  st::Everything::addSensor(&sensor19); 
  st::Everything::addSensor(&sensor20);

  //st::Everything::addSensor(&sensor21);
  //st::Everything::addSensor(&sensor22);
  //st::Everything::addSensor(&sensor23);
  //st::Everything::addSensor(&sensor24); 
  //st::Everything::addSensor(&sensor25); 
  //st::Everything::addSensor(&sensor26); 
  //st::Everything::addSensor(&sensor27); 
  //st::Everything::addSensor(&sensor28);
  
  st::Everything::addExecutor(&executor1);
    
  //*****************************************************************************
  //Initialize each of the devices which were added to the Everything Class
  //*****************************************************************************
  st::Everything::initDevices();
  //pinMode(LED_BUILTIN, OUTPUT);
}

//******************************************************************************************
//Arduino Loop() routine
//******************************************************************************************
void loop()
{
  //*****************************************************************************
  //Execute the Everything run method which takes care of "Everything"
  //*****************************************************************************
  //digitalWrite(LED_BUILTIN, HIGH);
  esp_task_wdt_feed();
  st::Everything::run();
  //digitalWrite(LED_BUILTIN, LOW);
  //delay(100);
}
