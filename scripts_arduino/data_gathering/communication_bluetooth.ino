#include <ArduinoBLE.h>           // Bluetooth Library
#include "DHT.h"
#define DHTPIN 7
#define DHTTYPE DHT11
#define HISTORY_LENGTH 128

DHT dht(DHTPIN, DHTTYPE);

inline void applyColor(int r, int g, int b) {
  analogWrite(LEDR, 255 - r);
  analogWrite(LEDG, 255 - g);
  analogWrite(LEDB, 255 - b);
}

String intsToAsciiString(const int *arr, size_t len) {
  String s;
  s.reserve(len);
  for (size_t i = 0; i < len; ++i) {
    s += (char)(arr[i] & 0xFF); // cast into ASCII
  }
  return s;
}

// Initalizing global variables for sensor data to pass onto BLE
float h, t;
String sh, st;
String history;

// BLE Service Name
BLEService customService("180C");

// BLE Characteristics
// Syntax: BLE<DATATYPE>Characteristic <NAME>(<UUID>, <PROPERTIES>, <DATA LENGTH>)
BLEStringCharacteristic ble_temperature("2A56", BLERead | BLENotify, 256);
BLEStringCharacteristic ble_humidity   ("2A57", BLERead | BLENotify, 256);
BLEStringCharacteristic ble_history0   ("2A58", BLERead | BLENotify, 256);
BLEStringCharacteristic ble_history1   ("2A59", BLERead | BLENotify, 256);


void setup()
{
    
    history = "";
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);

    applyColor(0, 0, 128);

    // Initalizing all the sensors
    dht.begin();
    Serial.begin(9600);
    while (!Serial);
    if (!BLE.begin())
    {
        Serial.println("BLE failed to Initiate");
        while (1);
    }

    // Setting BLE Name
    BLE.setLocalName("AES");
    
    // Setting BLE Service Advertisment
    BLE.setAdvertisedService(customService);
    
    // Adding characteristics to BLE Service Advertisment
    customService.addCharacteristic(ble_temperature);
    customService.addCharacteristic(ble_humidity);

    customService.addCharacteristic(ble_history0);
    customService.addCharacteristic(ble_history1);

    // Adding the service to the BLE stack
    BLE.addService(customService);

    // Start advertising
    BLE.advertise();
    Serial.println("Bluetooth device is now active, waiting for connections...");
    applyColor(0, 0, 255);

}

void loop()
{
  // Variable to check if cetral device is connected
  BLEDevice central = BLE.central();
  if (central)
  {   
    int i = 0;
    applyColor(0, 255, 0);
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    while (central.connected())
    {
      i++;
      delay(200);
      // Read values from sensors
      h = dht.readHumidity();
      t = dht.readTemperature();

      sh = String(h, 0);
      st = String(t, 0);

      if (i % 20 == 0) history += sh + " " + st + "|";

      // Writing sensor values to the characteristic
      ble_temperature.writeValue(sh);
      ble_humidity   .writeValue(st);


      const int payload0[] = {
        0x99,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09, // must start with 0x60 ou higher so that we can see the output in the nRF app
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19
      };
      String pld0 = intsToAsciiString(payload0, sizeof(payload0));
      ble_history0   .writeValue(pld0);


      const int payload1[] = {
        0x99, 0x20, 0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28, // must start with 0x60 ou higher so that we can see the output in the nRF app
        0x29, 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38
      };
      String pld1 = intsToAsciiString(payload1, sizeof(payload1));
      ble_history1   .writeValue(pld1);

      // Displaying the sensor values on the Serial Monitor
      Serial.println("Reading Sensors");
      Serial.println(sh + "% ");
      Serial.println(st + "°C");
      Serial.println("\n");
      applyColor(0, 128, 128);
      delay(50);
      applyColor(0, 0, 0);
      delay(50);
    }
  }

  applyColor(255, 0, 0);
  Serial.print("Disconnected from central: ");
  Serial.println(central.address());
}