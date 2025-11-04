#include "DHT.h"

#define DHTPIN 7
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();
  while (!Serial) {
    ; // wait for the serial connection to be established (useful with Nano 33)
  }
  Serial.println("Test Serial OK");
}

void loop() {

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  Serial.print("T = "); 
  Serial.print(t); 
  Serial.print(" °C, H = "); 
  Serial.print(h); 
  Serial.println(" %");
  delay(1000);
}