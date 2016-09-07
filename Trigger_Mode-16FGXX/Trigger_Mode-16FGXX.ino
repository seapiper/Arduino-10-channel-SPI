/*
   Copyright 2016 StretchSense Limited

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/*
 Circuit:
 16FGV1.0 sensor attached to pins 6, 7, 10 - 13:
 INTERUPT : pin 6
 NSS      : pin 7
 TRIGGER  : pin 8
 MOSI     : pin 11
 MISO     : pin 12
 SCK      : pin 13
 
 created 18 May 2016
 by Alan Deacon
 */

// the StretchSense circuit [16FGV1.0] communicates using SPI
#include <SPI.h>

// pins used for the connection with the sensor
// the other you need are controlled by the SPI library):
const int InteruptPin   =    6;
const int chipSelectPin =    7;
const int TriggerPin    =    8;

// ---- DEFINITIONS ----//

// Data package options
#define DATA    0x00  // 16FGV1.0 data packet
#define CONFIG  0x01  // 16FGV1.0 configuration command

// ODR - Output Data Rate
#define RATE_OFF             0x00
#define RATE_25HZ            0x01
#define RATE_50HZ            0x02
#define RATE_100HZ           0x03
#define RATE_166HZ           0x04
#define RATE_200HZ           0x05
#define RATE_250HZ           0x06
#define RATE_500HZ           0x07

// INT - Interrupt Mode
#define INTERRUPT_DISABLED    0x00
#define INTERRUPT_ENABLED     0x01

// TRG - Trigger Mode
#define TRIGGER_DISABLED     0x00
#define TRIGGER_ENABLED      0x01

// FILTER - Filter Mode
#define FILTER_1PT           0x01
#define FILTER_2PT           0x02
#define FILTER_4PT           0x04
#define FILTER_8PT           0x08
#define FILTER_16PT          0x10
#define FILTER_32PT          0x20
#define FILTER_64PT          0x40
#define FILTER_128PT         0x80
#define FILTER_255PT         0xFF

// RES - Resolution Mode
#define RESOLUTION_1pF       0x00
#define RESOLUTION_100fF     0x01
#define RESOLUTION_10fF      0x02
#define RESOLUTION_1fF       0x03

// Config Transfer
#define PADDING              0x00

// Configuration Setup
int   ODR_MODE        =      RATE_OFF;
int   INTERRUPT_MODE  =      INTERRUPT_DISABLED;
int   TRIGGER_MODE    =      TRIGGER_ENABLED;
int   FILTER_MODE     =      FILTER_1PT;
int   RESOLUTION_MODE =      RESOLUTION_10fF;

//SPI Configuration
SPISettings SPI_settings(2000000, MSBFIRST, SPI_MODE1); 

//Default scaling factor
int   CapacitanceScalingFactor = 100;
int   RawData[20];

///////////////////////////////////////////////////////////////
//  void loop()
//
//  @breif: 
//  @params: 
///////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(921600);

  //Initialise SPI port
  SPI.begin();
  SPI.beginTransaction(SPI_settings);

  // Initalize the  data ready and chip select pins:
  pinMode(InteruptPin, INPUT);
  pinMode(chipSelectPin, OUTPUT);
  pinMode(TriggerPin, OUTPUT);

  //Configure 16FGV1.0:
  writeConfiguration();
  //Get capacitance scaling factor
  CapacitanceScalingFactor = getCapacitanceScalingFactor(RESOLUTION_MODE);

  // give the circuit time to set up:
  delay(0.1);
}


///////////////////////////////////////////////////////////////
//  void loop()
//
//  @breif: 
//  @params: 
///////////////////////////////////////////////////////////////

void loop() {

  float capacitance =0;
  //Trigger a sample to begin
  digitalWrite(TriggerPin, HIGH);
  digitalWrite(TriggerPin, LOW);
  //Allow circuit to start sample
  delay(0.01);
  
   //Read the sensor Data
   readCapacitance(RawData);

   // convert the raw data to capacitance:
   for (int i=0; i<10; i++){
  
    capacitance = extractCapacitance(RawData,i);
    Serial.print(capacitance);  //Output capacitance values
    Serial.print(',');          //Output data as comma seperated values
  
  }
  
   Serial.print('\n');
    
}

///////////////////////////////////////////////////////////////
//  void writeConfiguration()
//
//  @breif: 
//  @params: 
///////////////////////////////////////////////////////////////
void writeConfiguration() {

  // 16FGV1.0 requires a configuration package to start streaming data

  // Set the chip select low to select the device:
  digitalWrite(chipSelectPin, LOW);

  SPI.transfer(CONFIG);                 //  Select Config Package
  SPI.transfer(ODR_MODE);               //  Set output data rate
  SPI.transfer(INTERRUPT_MODE);         //  Set interrupt mode
  SPI.transfer(TRIGGER_MODE);           //  Set trigger mode
  SPI.transfer(FILTER_MODE);            //  Set filter
  SPI.transfer(RESOLUTION_MODE);        //  Set Resolution
  for (int i=0;i<16;i++){
    SPI.transfer(PADDING);              //  Pad out the remaining configuration package
  }

  // take the chip select high to de-select:
  digitalWrite(chipSelectPin, HIGH);
}



///////////////////////////////////////////////////////////////
//  void readCapacitance(int raw[])
//
//  @breif: 
//  @params: int raw[] - Raw sensing data from 16FGV1.0
///////////////////////////////////////////////////////////////
void readCapacitance(int raw[]) {

  // 16FGV1.0 transmits data in the form of 10, 16bit capacitance values
  
  // Set the chip select low to select the device:
  digitalWrite(chipSelectPin, LOW);

  SPI.transfer(DATA);                   //  Select Data Package
  SPI.transfer(PADDING);                //  Get Sequence Number
  for (int i=0;i<20;i++){
    raw[i] =  SPI.transfer(PADDING);              //  Pad out the remaining configuration package
  }

  // take the chip select high to de-select:
  digitalWrite(chipSelectPin, HIGH);
  
}


///////////////////////////////////////////////////////////////
//  void setCapacitanceScalingFactor(int Resolution)
//
//  @breif: The 16FGV1.0 has an adjustable LSB resolution this function scales raw data to capacitance based on 
//  configuration settings.
//
//  @params: int Resolution_Config
///////////////////////////////////////////////////////////////
int getCapacitanceScalingFactor (int Resolution_Config)
{

  switch(Resolution_Config){
    case (RESOLUTION_1pF):
       return 1;  
    break;

    case (RESOLUTION_100fF):
       return 10;  
    break;

    case (RESOLUTION_10fF):
       return 100;  
    break;

    case (RESOLUTION_1fF):
       return 1000;  
    break;

  }

}

///////////////////////////////////////////////////////////////
//  float extractCapacitance(int[] raw, int channel)
//
//  @breif: 
//  @params: 
///////////////////////////////////////////////////////////////
float extractCapacitance(int raw[], int channel){

      float Capacitance =0;
  
      Capacitance = (raw[2*channel])*256+raw[2*channel+1];
      Capacitance /= CapacitanceScalingFactor;
    
      return Capacitance;
  
}


