/*
KLogger Code
Peter Marchetto, 07/2019, marchetto@umn.edu
Based on Nano Logger code by C. Fastie 12/2017

This code takes values from a BME280 T/RH/P sensor, a capacitive soil moisture sensor, a TCS230 color sensor, and a DS1307 RTC, concatenates them, and writes them via SPI to a µSD card on the Nano Logger board.
The parameters logged are time (both local and UTC), temperature in °C, pressure in hPa, relative humidity in %, soil moisture in LSBs, and red, green, blue, and white filtered spectral bands in arbitrary units (arbs).
Pin connections are as follows (all are 5 V tolerant, listed µC side first, device side second):
TCS230:
VCC to VCC, S0, and S1
GND to GND and OE (if present)
D2 to S2
D3 to S3
D4 to Out

Capacitive Soil Moisture Sensor:
VCC to VCC (Red)
GND to GND (Black)
A0 to Out (Yellow)

BME280:
VCC to VCC
GND to GND
A4 to SDA
A5 to SCL
*/

#include <SdFat.h>  //SDFat library for handling filesystems on SD cards
#include <SPI.h>  //SPI library for communicating with the SD card
#include <Wire.h> //Wire (I2C) library for communicating with the BME280
#include <RTClib.h> //Adafruit RTC library for handling the DS1307 on the Nano Logger board
#include <Adafruit_Sensor.h>  //Adafruit Unified Sensor library
#include <Adafruit_BME280.h>  //Adafruit BME280 library
#include <ArduinoUniqueID.h>  //Arduino Unique ID library
Adafruit_BME280 bme;  //BME280 object instance
#define BME280_I2C_ADDRESS 0x76 //The I2C address of the BME280 sensor is 0x76 or 0x77 (check both if yours isn't working)
RTC_DS1307 RTC;   //DS1397 RTC clock object instance
#define DS1307_I2C_ADDRESS 0x68 //The I2C address of the RTC is 0x68
SdFat SD;   //SdFat object instance
#define MOSIpin 11  //SPI MOSI pin
#define MISOpin 12  //SPI MISO pin
#define S2 2  //S2 filter select pin on the TCS230 light sensor
#define S3 3  //S3 filter select pin on the TCS230 light sensor
#define sensorOut 4 // Output pin on the TCS230 light sensor
const int chipSelect = 10;  //SPI CS pin for the SD card
char tmeStrng[ ] = "0000/00/00,00:00:00"; //Template for a date/time string
float bmet; //Variable for storing temperature
float bmep; //Variable for storing barometric pressure
float bmerh;  //Variable for storing relative humidity
int soil; //Variable for storing soil moisture
int r;  //Variable for storing red value
int g;  //Variable for storing green value
int b;  //Variable for storing blue value
int w;  //Variable for storing white value
long utc; //Variable for storing UTC time
long ID; //Variable to hold the unique ID of the ATMEGA328p chip on the Arduino
int logSeconds = 5; //Time in seconds between logging events
long logMillis = logSeconds * 1000; //Calculates time between logging events in milliseconds, which are compatible with the delay() function
             
void setup() {  //Setup function
  Serial.begin(9600);  // Open serial communications at 9600 bps
  for (size_t i = 0; i < 8; i++){ //For loop for running through the eight registers of the unique ID
    ID+=(UniqueID8[8-i]*256^(i+1)); //Add each byte as a hexadecimal place to the ID
  }
  Wire.begin();  //Initialize the I2C interface
  RTC.begin(); //Initialize the RTC 
  RTC.adjust(DateTime((__DATE__), (__TIME__))); //Sets the RTC to the time the sketch was compiled
  Serial.println(ID); //Display the unique ID
  Serial.print("Find SD card: "); //Say what we're doing with the SD card
  if (!SD.begin(chipSelect)) { //Initialize and display the status of the SD card
    Serial.println("Card failed");  //Say that we can't talk to the SD card if we can't
    while(1); //Keep trying
  }
  Serial.println(" SD card OK");  //Display if SD card is ok
  Serial.print("Logging to microSD card every "); //Prompt that we're logging
  Serial.print(logSeconds); //Say how often we're logging
  Serial.println(" seconds.");  //Specify units
  bme.begin(BME280_I2C_ADDRESS);  //Start the BME280
  delay(2000);  //Wait so the sensor can initialize 
  pinMode(S2, OUTPUT);  //Set the pin mode for the S2 pin as output
  pinMode(S3, OUTPUT);  //Set the pin mode for the S  //Set the pin mode for the S2 pin as output pin as output
  pinMode(sensorOut, INPUT);    //Set the pin mode for the TCS230 output pin as input
  File dataFile = SD.open("datalog.txt", FILE_WRITE); //Start a data file on the SD card
  dataFile.println(ID); //Write the unique ID on the first line of the file
  dataFile.println("Time,UTC,T (°C),P (hPa),RH (%),Soil Moisture (LSBs),Red (arbs),Green (arbs),Blue (arbs),White (arbs)");  //Print a header to the data file with column headings
  dataFile.flush(); //Wait for SD write to finish
  dataFile.close(); //Close the data file
}

void loop() 
{
  bmet = (bme.readTemperature()); //Read the temperature in °C
  bmep = (bme.readPressure() / 100.0F); //Read the barometric pressure in hPa
  bmerh = (bme.readHumidity()); //Read the humidity in %RH
  soil = analogRead(0); //Read the soil moisture in LSBs
  digitalWrite(S2,LOW); //Set the S2 line to LOW
  digitalWrite(S3,LOW); //Set the S3 line to LOW
  r = pulseIn(sensorOut, LOW);  //Read the pulse length for the red filtered TCS230 channel
  digitalWrite(S2,HIGH);  //Set the S2 line to HIGH
  digitalWrite(S3,HIGH);  //Set the S3 line to HIGH
  g = pulseIn(sensorOut, LOW);  //Read the pulse length for the green filtered TCS230 channel
  digitalWrite(S2,LOW); //Set the S2 line to LOW
  digitalWrite(S3,HIGH);  //Set the S3 line to HIGH
  b = pulseIn(sensorOut, LOW);  //Read the pulse length for the blue filtered TCS230 channel
  digitalWrite(S2,HIGH);  //Set the S2 line to HIGH
  digitalWrite(S3,LOW); //Set the S3 line to LOW
  w = pulseIn(sensorOut, LOW);  //Read the pulse length for the unfiltered TCS230 channel
  DateTime now = RTC.now(); //Read the time from the RTC
  utc = (now.unixtime()); //Fill the UTC variable from the RTC
  sprintf(tmeStrng, "%04d/%02d/%02d,%02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()); //Concatenate parts into a full time string
  Serial.print("RTC utc Time: "); //Say what we're displaying  
  Serial.println(now.unixtime()); //Display the variable contents
  Serial.print("RTC time: "); //Say what we're displaying
  Serial.println(tmeStrng); //Display the variable contents
  Serial.print("BME280 temp: ");  //Say what we're displaying
  Serial.print(bmet); //Display the variable contents
  Serial.println(" C"); //Display units
  Serial.print("Pressure: "); //Say what we're displaying
  Serial.print(bmep); //Display the variable contents
  Serial.println(" hPa"); //Display units
  Serial.print("Humidity: "); //Say what we're displaying
  Serial.print(bmerh);  //Display the variable contents
  Serial.println(" %RH"); //Display units
  Serial.print("Soil Moisture: ");  //Say what we're displaying
  Serial.print(soil); //Display the variable contents
  Serial.println(" LSBs");  //Display units
  Serial.print("Light levels (R,G,B,W): "); //Say what we're displaying
  Serial.print(r);  //Display variable contents for red
  Serial.print(",");  //Send a comma
  Serial.print(g);  //Display variable contents for green
  Serial.print(",");  //Send a comma
  Serial.print(b);  //Display variable contents for blue
  Serial.print(",");  //Send a comma
  Serial.print(w);  //Display variable contents for white
  Serial.println(" arbs");  //Display units
  Serial.println(); //Skip a line
  File dataFile = SD.open("datalog.txt", FILE_WRITE); //Open the data file on the SD card
  dataFile.print(tmeStrng); //Write the time
  dataFile.print(",");  //Write a comma
  dataFile.print(utc);  //Write the UTC time string
  dataFile.print(",");  //Write a comma
  dataFile.print(bmet); //Write the temperature
  dataFile.print(",");  //Write a comma
  dataFile.print(bmep); //Write the barometric pressure
  dataFile.print(",");  //Write a comma
  dataFile.print(bmerh);  //Write the relative humidity
  dataFile.print(",");  //Write a comma
  dataFile.print(soil); //Write the soil moisture
  dataFile.print(",");  //Write a comma
  dataFile.print(r);  //Write the red channel value
  dataFile.print(",");  //Write a comma
  dataFile.print(g);  //Write the green channel value
  dataFile.print(",");  //Write a comma
  dataFile.print(b);  //Write the blue channel value
  dataFile.print(",");  //Write a comma
  dataFile.println(w);  //Write the white channel value and end the line
  dataFile.flush(); //Wait for SD write to finish
  dataFile.close(); //Close the data file
  delay(logMillis); //Wait for logMillis milliseconds before taking measurements again and logging the next entry
} 
