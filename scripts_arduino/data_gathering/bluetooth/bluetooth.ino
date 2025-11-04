#include <ArduinoBLE.h>           // Bluetooth Library
#include "DHT.h"
#define DHTPIN 7
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// Initalizing global variables for sensor data to pass onto BLE
float h, t;
String sh, st;

// BLE Service Name
BLEService customService("180C");

// BLE Characteristics
// Syntax: BLE<DATATYPE>Characteristic <NAME>(<UUID>, <PROPERTIES>, <DATA LENGTH>)
BLEStringCharacteristic ble_temperature("2A56", BLERead | BLENotify, 8);
BLEStringCharacteristic ble_humidity   ("2A57", BLERead | BLENotify, 8);


void setup()
{
    

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
    BLE.setLocalName("Arduino Environment Sensor");
    
    // Setting BLE Service Advertisment
    BLE.setAdvertisedService(customService);
    
    // Adding characteristics to BLE Service Advertisment
    customService.addCharacteristic(ble_temperature);
    customService.addCharacteristic(ble_humidity);

    // Adding the service to the BLE stack
    BLE.addService(customService);

    // Start advertising
    BLE.advertise();
    Serial.println("Bluetooth device is now active, waiting for connections...");
}

void loop()
{
    // Variable to check if cetral device is connected
    BLEDevice central = BLE.central();
    if (central)
    {
        Serial.print("Connected to central: ");
        Serial.println(central.address());
        while (central.connected())
        {
            delay(200);
            // Read values from sensors
            h = dht.readHumidity();
            t = dht.readTemperature();
            sh = String(h, 2) + "%";
            st = String(t, 2) + "C";

            // Writing sensor values to the characteristic
            ble_temperature.writeValue(sh);
            ble_humidity   .writeValue(st);

            // Displaying the sensor values on the Serial Monitor
            Serial.println("Reading Sensors");
            Serial.println(h);
            Serial.println(t);
            Serial.println("\n");
            delay(1000);
        }
    }
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
}