// Inspired from https://github.com/pili-zhangqiu/Wireless-PC-Communication-with-the-Arduino-Nano-33-Series?tab=readme-ov-file

#include <ArduinoBLE.h>
#include "DHT.h"

#define DHTPIN 7
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// ------------------------------------------ BLE UUIDs ------------------------------------------
//must match BLE_IMU_CENTRAL
#define BLE_UUID_PERIPHERAL               "19B10000-E8F2-537E-4F6C-D104768A1214" 
#define BLE_UUID_CHARACT_LED              "19B10001-E8F2-537E-4F6C-E104768A1214" 
#define BLE_UUID_CHARACT_ACCX             "29B10001-E8F2-537E-4F6C-a204768A1215"
#define BLE_UUID_CHARACT_ACCY             "39B10001-E8F2-537E-4F6C-a204768A1215" 
#define BLE_UUID_CHARACT_ACCZ             "49B10001-E8F2-537E-4F6C-a204768A1215"

BLEService LED_IMU_Service(BLE_UUID_PERIPHERAL); // BLE LED Service

// BLE LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEByteCharacteristic  ch1(BLE_UUID_CHARACT_LED, BLERead | BLEWrite);
BLEStringCharacteristic ch2("2A56", BLERead | BLENotify, 256);
BLEStringCharacteristic ch3("2A57", BLERead | BLENotify, 256);
BLEStringCharacteristic ch4("2A58", BLERead | BLENotify, 256);


const int ledPin = LED_BUILTIN; // pin to use for the LED
float x, y, z;
String shumidity, stemperature, stime;

// ------------------------------------------ VOID SETUP ------------------------------------------
void setup() {
  shumidity = "";
  stemperature = "";
  stime = "";
  Serial.begin(9600); 
  //while (!Serial); //uncomment to view the IMU data in the peripheral serial monitor

  dht.begin();

  // set LED pin to output mode
  pinMode(ledPin, OUTPUT);

  // begin BLE initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  BLE.setLocalName("AES"); // Arduino Environment Sensors
  BLE.setAdvertisedService(LED_IMU_Service);

  // add the characteristic to the service
  LED_IMU_Service.addCharacteristic(ch1);
  LED_IMU_Service.addCharacteristic(ch2);
  LED_IMU_Service.addCharacteristic(ch3);
  LED_IMU_Service.addCharacteristic(ch3);


  // add service
  BLE.addService(LED_IMU_Service);

  // set the initial value for the characeristic:
  ch1.writeValue(0);


  // start advertising
  BLE.advertise();

  Serial.println("BLE LED Peripheral");
}

// ------------------------------------------ VOID LOOP ------------------------------------------
void loop() {
  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    // while the central is still connected to peripheral:
    while (central.connected()) {
      // if the remote device wrote to the characteristic,
      // use the value to control the LED:
      if (ch1.written()) {
        if (ch1.value()) {   // any value other than 0
          Serial.println("LED on");
          digitalWrite(ledPin, HIGH);         // will turn the LED on
        } else {                              // a 0 value
          Serial.println(F("LED off"));
          digitalWrite(ledPin, LOW);          // will turn the LED off
        }
      }

      x = millis() / 1000.0;
      y = dht.readHumidity();
      z = dht.readTemperature();

      stemperature += String(z, 2);
      shumidity += String(y,2);
      stime += String(x,2);


      ch2.writeValue(stemperature);
      ch3.writeValue(shumidity);
      ch3.writeValue(stime);

      Serial.print(stemperature); 
      Serial.print('\t');
      Serial.print(shumidity); 
      Serial.print('\t');         
      Serial.print(stime); 
      Serial.println('\t'); 

    }

      // when the central disconnects, print it out:
      Serial.print(F("Disconnected from central: "));
      Serial.println(central.address());
    }
  }
