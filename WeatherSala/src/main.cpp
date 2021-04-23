#include <Arduino.h>
#include <Wire.h>   
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h> 


#define TEMPID 1
#define PRESSID 2
#define HUMID 3

//#define MY_DEBUG

#define MY_RADIO_RF24
#define MY_RF24_CHANNEL 107
#define MY_REPEATER_FEATURE
#define LONG_WAIT 500
#define SHORT_WAIT 50

#include <MySensors.h>


MyMessage temp_msg  (TEMPID, V_TEMP);
MyMessage press_msg (PRESSID, V_PRESSURE);
MyMessage hum_msg   (HUMID, V_HUM);

void presentation(){
  sendSketchInfo("WeatherSala","0.1");

  present(TEMPID, S_TEMP,"TempSala");
  wait(SHORT_WAIT);
  present(PRESSID, S_BARO, "PressSala");
  wait(SHORT_WAIT);
  present(HUMID, S_HUM, "HumSala");
  wait(LONG_WAIT);

}


Adafruit_BME280 bme;

void sendValues() {
    Serial.print("Temperature = ");
    float temp = bme.readTemperature();
    send(temp_msg.set(temp, 4));
    Serial.print(temp);
    Serial.println(" *C");

    Serial.print("Pressure = ");
    float press = bme.readPressure() / 100.0F;
    send(press_msg.set(press,4));
    Serial.print(press);
    Serial.println(" hPa");

    Serial.print("Humidity = ");
    float hum = bme.readHumidity();
    send(hum_msg.set(hum,2));
    Serial.print(hum);
    Serial.println(" %");

    Serial.println();
}

void setup()
{
  Serial.begin(115200);     
    
  if(!bme.begin()){
    Serial.print("Couldn't find BME");
    while(1);
  }  
  bme.setSampling();
}

void loop() {
  // put your main code here, to run repeatedly:
  sendValues();
  wait(60000*5);
}