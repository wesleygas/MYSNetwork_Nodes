#include <Arduino.h>

unsigned long readings[4];
#define LampID 5
#define VCCID 4
#define TEMPID 3
#define LIGHTID 2
#define SOILID 1

#define SOILPIN A0
#define LIGHTPIN A1
#define TEMPPIN A2
#define VCCPIN A3
#define LampPin 5

#define SensBus 4

#define ADCres 1.061/1024.0//0.00107421875

#define MIN_BAT 3.0f


/* 5v rail voltage divider ratio (Rbot/Rtop): .2524 | (11.72k/46.7k) Rpot/Rbot = 3.961
 x = analogRead

 Rail voltage = x * (1.1V/1024 steps)* 3.961

*/

/* Thermistor readings:

 Rtop = 68k
 x = analogread
Vadc = (1.1V/1024) * x
R = (Vadc*68k)/(Vin-Vadc)

 Probable offset -2ºC

 - °C | R
  8.9	  36
  10.82	30
  12.3	25
  14.4	21


   ºC = -5.3429*R + 42.2

*/

/* LDR resistor: 47K

Value = log(1023.0/reading)

*/

byte lampBrightness = 0;

float calcTemp(float read, float &Vin){
  float Vadc = ADCres*read;
  float R = (Vadc*68.0f/(Vin-Vadc));
  return -2.764f*R + 60.07f;
}

float calcRailVoltage(float read){
  return read*ADCres*(3.961f+1.0f);
}

float calcLDR(float read){
  return 1023.0/read;
}




//#define MY_DEBUG

#define MY_RADIO_RF24
#define MY_RF24_CHANNEL 107
//#define MY_REPEATER_FEATURE
#define LONG_WAIT 500
#define SHORT_WAIT 50


// #define EEPROM_DIM_LEVEL_LAST 1
// #define EEPROM_DIM_LEVEL_SAVE 2




#include <MySensors.h>

// MyMessage lampperc_msg(LampID,V_PERCENTAGE);
// MyMessage lampstat_msg(LampID, V_STATUS);
MyMessage vcc_msg(VCCID,V_VOLTAGE);
MyMessage temp_msg(TEMPID, V_TEMP);
MyMessage light_msg(LIGHTID, V_LEVEL);
MyMessage soil_msg(SOILID, V_LEVEL);

void before()
{
  pinMode(SensBus,OUTPUT);
  pinMode(LampPin,OUTPUT);
  analogWrite(LampPin,0);
  analogReference(INTERNAL);
  for(int i = A0; i <= A3; i++){
    pinMode(i,INPUT);
  }
}

void presentation(){
  sendSketchInfo("PlantStation", "0.2");
  wait(SHORT_WAIT);
  // present(LampID, S_DIMMER, "Lamp");
  // wait(SHORT_WAIT);
  present(VCCID, S_MULTIMETER, "Vbat");
  wait(SHORT_WAIT);
  present(TEMPID, S_TEMP, "SoilTemp");
  wait(SHORT_WAIT);
  present(LIGHTID,S_LIGHT_LEVEL, "LightLevel");
  wait(SHORT_WAIT);
  present(SOILID, S_MOISTURE, "SoilHum");
  wait(LONG_WAIT);

  // Serial.print("Send initial valuelamp - ");
  // send(lampperc_msg.set((int16_t)10));
  // send(lampstat_msg.set((int16_t)1));
  // request(LampID, V_PERCENTAGE);
  // wait(2000,C_SET, V_PERCENTAGE);
  // Serial.println("Value sent");

  // Serial.print("V Temperature: ");
  // float temp = calcTemp((float)analogRead(TEMPPIN), vcc);
  // send(temp_msg.set(temp,3));
  // request(TEMPID,V_TEMP);
  // wait(2000, C_SET, V_TEMP);
  // Serial.print(temp);

  // Serial.print("oC Light Level: ");
  // float light = calcLDR((float)analogRead(LIGHTPIN));
  // send(light_msg.set(light,3));
  // request(LIGHTID,V_LEVEL);
  // wait(2000, C_SET, V_LEVEL);
  // Serial.print(light);

  // Serial.print(" Soil: ");
  // float hum = (float)analogRead(SOILPIN);
  // send(soil_msg.set(hum,3));
  // request(SOILID, V_LEVEL);
  // wait(2000, C_SET, V_LEVEL);
  // Serial.println(hum);

}

// void receive(const MyMessage &message)
// {
//   if(message.getSensor() == LampID){
//     int16_t fadeTo = 0;
//     if (message.type == V_PERCENTAGE) {
//       // Incoming dim-level command sent from controller (or ack message)
//       fadeTo = message.getInt();
//       // Save received dim value to eeprom (unless turned off). Will be
//       // retreived when a on command comes in
//     }else if(message.type == V_STATUS){
//       fadeTo = (message.getInt()==0)?0:50;
//     }
//     fadeTo = constrain(fadeTo,0,100);
//     Serial.print("New light level received: ");
//     Serial.println(fadeTo);

//     // Stard fading to new light level
//     float vbat = calcRailVoltage((float)analogRead(VCCPIN));
//     if(vbat < MIN_BAT){
//       fadeTo = 0;
//       Serial.println("Battery too low!");
//     }
//     analogWrite(LampPin, fadeTo*(255.0f/100.0f));
//     send(lampperc_msg.set(fadeTo));
//     send(lampstat_msg.set(fadeTo));
//   }
// }


void setup() {
  //Serial.begin(115200);
  for(int i = 0; i < 4; i++){
    readings[i] = 0;
  }
  
  
}

unsigned long start = 0;

void loop() {
  int count = 0;
  digitalWrite(SensBus, 1);
  start = millis();
  while(millis() - start < 2000){
    for(int i = A0; i <= A3; i++){
     readings[i-A0] += analogRead(i);
    }
    count++;
  }
  digitalWrite(SensBus,0);

  float Vin = calcRailVoltage((float)readings[3]/(float)count);
  readings[3] = 0;
  float Temperature = calcTemp((float)readings[2]/(float)count,Vin);
  readings[2] = 0;
  float Light = calcLDR((float)readings[1]/(float)count);
  readings[1] = 0;
  float soil = (float)readings[0]/(float)count;
  readings[0] = 0;

  bool success = false;
  Serial.print("Vin: ");
  Serial.print(Vin);
  success &= send(vcc_msg.set(Vin,4));
  Serial.print(" Temp: ");
  Serial.print(Temperature);
  success &= send(temp_msg.set(Temperature,4));
  Serial.print(" Light: ");
  Serial.print(Light);
  success &= send(light_msg.set(Light,4));
  Serial.print(" Soil: ");
  Serial.print(soil);
  success &= send(soil_msg.set(soil,4));
  Serial.print(" Samples: ");
  Serial.println(count);
  

  
  sleep(60000*5, false);
}