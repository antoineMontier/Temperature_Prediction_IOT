#include <ArduinoBLE.h>
#include "DHT.h"

#define DHTPIN 7
#define DHTTYPE DHT11
#define MAX_BLUETOOTH_SIZE 128
#define ARRAY_SIZE 20
#define BLE_NAME "AES"

#define FREQUENCY_OF_SENDING 400 // ms
#define LENGTH_OF_SENDING 60 // s
#define SAMPLING_DELAY 30 // m
#define FREQUENCY_OF_CHEKING_BLE 5 // s

DHT dht(DHTPIN, DHTTYPE);

// BLE LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEStringCharacteristic characteristic_temp       ("2A56", BLERead | BLENotify | BLEWrite, MAX_BLUETOOTH_SIZE);
BLEStringCharacteristic characteristic_temp_digits("2A57", BLERead | BLENotify | BLEWrite, MAX_BLUETOOTH_SIZE);
BLEStringCharacteristic characteristic_humi       ("2A58", BLERead | BLENotify | BLEWrite, MAX_BLUETOOTH_SIZE);
BLEStringCharacteristic characteristic_time       ("2A59", BLERead | BLENotify | BLEWrite, MAX_BLUETOOTH_SIZE);

// BLE service name
BLEService env_service("180C");

typedef struct SensorData {
    int temperature;
    int humidity;
    int temp_digits;
    int time;
} SensorData;

typedef struct HistoryData {
    int temperature[ARRAY_SIZE];
    int humidity[ARRAY_SIZE];
    int temp_digits[ARRAY_SIZE];
    int time[ARRAY_SIZE];
    int index;
} HistoryData;

HistoryData history;
float temp, humidity;


int intToBCD (int v);
inline void applyColor (int r, int g, int b);
SensorData get_data ();
HistoryData intialize_history ();
void history_offset_1 (HistoryData * history);
void update_history_data (HistoryData * history);
void send_data (const HistoryData history);
String intsToAsciiString(const int *arr, size_t len);

void setup() {
    Serial.println("\n\n\n");
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);
    applyColor(0, 0, 128);

    history = intialize_history();
    Serial.begin(9600); 

    dht.begin();

    // begin BLE initialization
    if (!BLE.begin()) {
        Serial.println("starting BLE failed!");
        applyColor(255, 25, 25); // error LED
        while (1);
    }

    BLE.setLocalName(BLE_NAME); // Arduino Environment Sensors

    BLE.setAdvertisedService(env_service);

    // add the characteristic to the service
    env_service.addCharacteristic(characteristic_temp       );
    env_service.addCharacteristic(characteristic_temp_digits);
    env_service.addCharacteristic(characteristic_humi       );
    env_service.addCharacteristic(characteristic_time       );

    // add service
    BLE.addService(env_service);

    // start advertising
    BLE.advertise();
    applyColor(0, 0, 255);
    Serial.println("Service is running !");
}

void loop() {
    while(1){

        update_history_data(&history);
        Serial.println("[SEN] Reading sensors");

        SensorData sd = get_data();
        Serial.println("     temp: " + String(sd.temperature) + " temp_digits: " + String(sd.temp_digits) + " humidity: " + String(sd.humidity) + " time: " + String(sd.time));

        for (int i = 0; i < ARRAY_SIZE; ++i) {
            Serial.print  ("      Temperature: 0x" + String(history.temperature[i], HEX));
            Serial.print  (" Temperature_digits: 0x" + String(history.temp_digits[i], HEX));
            Serial.print  (" Humidity: 0x" + String(history.humidity[i], HEX));
            Serial.println(" Time: 0x" + String(history.time[i], HEX));
        }

        for (int second = 0 ; second < SAMPLING_DELAY * 60 ; second += FREQUENCY_OF_CHEKING_BLE){
        
            // check every n seconds if someone is trying to connect
            // send data in a loop for x seconds with an interval of y
            BLEDevice central = BLE.central();
            if(central.connected()){
                Serial.println("[BLE] Connected to central: " + central.address() + " Sending data every " + String(FREQUENCY_OF_SENDING) + "ms for about " + String(LENGTH_OF_SENDING) + "s...");
                for (int ms = 0 ; ms < LENGTH_OF_SENDING * 1000 ; ms+=FREQUENCY_OF_SENDING){
                    send_data(history);
                    applyColor(0, 128, 128);
                    delay(int(FREQUENCY_OF_SENDING/2));
                    applyColor(0, 0, 0);
                    delay(int(FREQUENCY_OF_SENDING/2));
                    second += ms; // if someone connects, we lose LENGTH_OF_SENDING seconds so we must remove them from the timer
                    if(!central.connected()){
                      Serial.println("[BLE] Communication with:" + central.address() + " has terminated");
                      break;
                    }
                }
                Serial.println("[BLE] Communication with: " + central.address() + " is considered finished.");

            }
            
            delay(FREQUENCY_OF_CHEKING_BLE * 1000 - 100); // frequency of waiting in seconds
            applyColor(0, 60, 128);
            delay(100);
            applyColor(0, 0, 0);

        }
    }
}

inline void applyColor(int r, int g, int b) {
  analogWrite(LEDR, 255 - r);
  analogWrite(LEDG, 255 - g);
  analogWrite(LEDB, 255 - b);
}

int intToBCD(int v) {
  if (v < 0) v = 0;
  if (v > 99) v = v % 100;
  return (int)(((v / 10) << 4) | (v % 10));
}

SensorData get_data ()
{
    SensorData res;
    temp     = dht.readTemperature();
    res.temperature = int(temp);
    res.temp_digits = int((temp - res.temperature ) * 100);
    res.humidity = int(dht.readHumidity());
    res.time = int(millis()) / (1000 * 60 * 30) + 1 ; // half-hours // +1 because 0x00 terminates the string
    
    return res;
}

HistoryData intialize_history ()
{
    HistoryData res;
    for (int i = 0 ; i < ARRAY_SIZE ; ++i ) // intialize with random values
    {
        res.humidity[i] = 0xFF;
        res.temperature[i] = 0xFF;
        res.temp_digits[i] = 0xFF;
        res.time[i] = 0xFF;
    }
    res.index = 1; // let the first one at 0xFF

    return res;
}

void history_offset_1 (HistoryData * history)
{
    for (int i = 1 ; i < ARRAY_SIZE ; ++i){ // shift everything in the history by one step
        history->humidity[i-1]    = history->humidity[i];
        history->temperature[i-1] = history->temperature[i];
        history->temp_digits[i-1] = history->temp_digits[i];
        history->time[i-1]        = history->time[i];
    }

    history->humidity   [ARRAY_SIZE - 1] = 0xFF;
    history->temperature[ARRAY_SIZE - 1] = 0xFF;
    history->temp_digits[ARRAY_SIZE - 1] = 0xFF;
    history->time       [ARRAY_SIZE - 1] = 0xFF;
}

void update_history_data (HistoryData * history) 
{
    // overwrite the last value
    history_offset_1(history);
    
    // obtain values
    SensorData data = get_data();

    // add to history
    if (data.humidity != 0){
        history->humidity   [ARRAY_SIZE - 1] = data.humidity;
    }else{
        history->humidity   [ARRAY_SIZE - 1] = 0xFE;
    }
    history->temperature[ARRAY_SIZE - 1] = data.temperature;
    history->temp_digits[ARRAY_SIZE - 1] = data.temp_digits;
    history->time       [ARRAY_SIZE - 1] = data.time;
}

String intsToAsciiString(const int *arr, size_t len)
{
    String s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        s += (char)(arr[i] & 0xFF); // cast to ASCII
    }
    return s;
}


void send_data (const HistoryData history)
{
    // transform history into ASCII before sending    
    characteristic_temp       .writeValue(intsToAsciiString(history.temperature, sizeof(history.temperature)));
    characteristic_temp_digits.writeValue(intsToAsciiString(history.humidity   , sizeof(history.humidity   )));
    characteristic_humi       .writeValue(intsToAsciiString(history.temp_digits, sizeof(history.temp_digits)));
    characteristic_time       .writeValue(intsToAsciiString(history.time       , sizeof(history.time       )));
}