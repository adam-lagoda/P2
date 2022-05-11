#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
/*GPS Module is connect as following;
VCC = 3.3V Pin
GND = GND
TX = Pin 4
RX = Pin 3
*/
static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

void setup(){
  Serial.begin(9600);
  ss.begin(GPSBaud);
  while (!Serial);

  Serial.println("Beacon");
  
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

}

void loop(){
  // This part displays information every time a new sentence is correctly encoded from the GPS Module and is being sent through the LoRa module.
  while (ss.available() > 0){
    gps.encode(ss.read());
    }
    if (gps.location.isUpdated()){
      LoRa.beginPacket();
      LoRa.print(gps.location.lat(), 6);
      LoRa.print(";");
      LoRa.print(gps.location.lng(), 6);
      LoRa.print(";");
      LoRa.endPacket();
    }
}