#include <SPI.h>
#include <Wire.h>
#include <LSM303.h>
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

//Defining all the neccessary global variables;

String incomingString = "";
String osdString = "";

float longitudeBeacon;
float latitudeBeacon;

float longitudeReceiver;
float latitudeReceiver;

float distance=0;
float angleReceiver=0;
float angle=0;
float aimAngle=0;
float height=0;
float batteryLevel;
int batteryLevelRead;

long distanceLong=0;
long aimAngleLong=0;
long heightLong=0;
long batteryLevelLong;

int counter=0;
int counterGPSLoop = 0;

//The LSM303D object
LSM303 compass;

// The TinyGPS++ object
TinyGPSPlus gps;

// The software serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

void setup() 
{
  Serial.begin(9600);
  ss.begin(GPSBaud);

  Wire.begin();
  compass.init();
  compass.enableDefault();
  compass.m_min = (LSM303::vector<int16_t>){-1618, -1499, -5036}; //Calibrating the minimum and maximum values of magnetometer readings.
  compass.m_max = (LSM303::vector<int16_t>){+4274, +4236, +1579};
  
  while (!Serial);
  Serial.println("Receiver");
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}
 
void loop() {
  while (ss.available() > 0){
    gps.encode(ss.read());
    }
    if (gps.location.isUpdated()){
      longitudeReceiver =((float)gps.location.lng()); //Saving receiver's coordinates and altitude in the global variables
      latitudeReceiver = ((float)gps.location.lat());
      height=((float)gps.altitude.meters());
      
    }
  int packetSize = LoRa.parsePacket();
  if (packetSize){
    while (LoRa.available()) { //Fragmenting script to divide the incoming String into 2 float variables (Beacon's coordinates)
      char incoming = (char)LoRa.read();
      if(isDigit(incoming) && incoming != ';'){
        incomingString += (char)incoming;
      }
      if(incoming == ';'){
        if(counter ==0){
          latitudeBeacon = (incomingString.toFloat())/1000000.0;
          counter++;
          incomingString="";
        }else{
          longitudeBeacon = (incomingString.toFloat())/1000000.0;
          counter=0;
          incomingString="";
        }
      }
    }
    
    distanceCalc(latitudeBeacon, longitudeBeacon, latitudeReceiver, longitudeReceiver); // Calculating the distance between the Beacon and the Receiver
    angleCalc(latitudeBeacon, longitudeBeacon, latitudeReceiver, longitudeReceiver); //Calculating the angle between the Beacon and the Receiver AND the direction to the North Pole 
    
    compass.read(); //reading where the drone is facing in the correlation to the North Pole
    angleReceiver = compass.heading();
    aimAngle = angleReceiver+angle-90;

    batteryLevelRead = analogRead(A0); //Mapping the battery's voltage from the analog value read to the value of the battery volateg * 10
    batteryLevel = map(batteryLevelRead, 0 , 1023, 0, 252);

    distanceLong = (unsigned long)distance; //Converting all the variables that are being later sent to the Mavlink to unsigned long
    aimAngleLong = (unsigned long)aimAngle;
    heightLong = (unsigned long)heightLong;
    batteryLevelLong = (unsigned long)batteryLevel;


    
    Serial.print("/"); //Sednign a packet of data to the mavlink
    Serial.print(distanceLong);
    Serial.print(";");
    Serial.print(aimAngleLong);
    Serial.print(";");
    Serial.print(heightLong);
    Serial.print(";");
    Serial.print(batteryLevelLong);
    Serial.println(";");
  
}
}

void distanceCalc(float lat1, float lng1, float lat2, float lng2){ //Function that calculates the distance between two points on the Earth.
  float dlat=(40030170.0*(abs(lat1-lat2)))/360.0;
  float dlong=(40030170.0*(abs(lng1-lng2))*cos((lat1+lat2)/2.0))/360.0;
  distance=sqrt(pow(dlat,2)+pow(dlong,2));
  return distance;
}
void angleCalc(float lat1, float lng1, float lat2, float lng2){//Function that calculates angle between the Beacon and the Receiver AND the direction to the North Pole 
  float dlat=(40030170.0*(abs(lat1-lat2)))/360.0;
  float dlong=(40030170.0*(abs(lng1-lng2))*cos((lat1+lat2)/2.0))/360.0;
  angle=atan(dlong/dlat);
  return angle;
}