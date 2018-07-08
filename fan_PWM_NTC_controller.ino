/*
This system consumes at most between 45mA and 230mA, depends on TM 1638 brightness and how many numbers are being shown

--- Pinout Schema ---
3v3   -> 10K resistor -> 10K NTC -> GND
A0    -> 10K resistor -> 10K NTC -> GND
AREF -> 3V3 (jumper)
*/


#include <TM1638.h>
/* --- Pins defines --- */
#define THERMISTORPIN A0          // which analog pin to connect
#define THERMISTORNOMINAL 10000   // resistance at 25 degrees C (in Ohms)
#define TEMPERATURENOMINAL 25     // temp. for nominal resistance (almost always 25 C)
#define NUMSAMPLES 10             // how many samples to take and average, more takes longer but is more 'smooth'
#define BCOEFFICIENT 4000         // The beta coefficient of the thermistor (usually 3000-4000)
#define SERIESRESISTOR 9600       // the value of the 'other' resistor (in Ohms)
#define DELAY 50                  // delay in miliseconds between each sample measurement
/* --- Controller Pins and Vars --- */
#define FANPWM 3                  // set pin for fan PWM
#define FANSENSOR 4               // set pin for fan sensor (tachometer)
#define PULSE 2                   // set number of pulse 
int fanControl = 0;               // var for control the fan speed
int fanRPM = 0;                   // var that count RPM
float tempTarget = 12;            // temperature target
unsigned long sensorPulse;
/* ---------------------- */
/* --- Peltier--- */
#define PELTIER 5                 // The pin that the peltier MOSFET is connected
int peltierControl = 0;           // var used to set the peltier power
/* ---------------------- */
/* --- TM1638 Display --- */
int brightness = 4;                           // default TM1638 brightness
TM1638 module(8, 9, 10, true, brightness);    // Data, Clock, Strobe, DisplayActivate, Intensity
/* ---------------------- */

uint16_t samples[NUMSAMPLES];     // array for the samples
 
void setup(void) {
  Serial.begin(9600);
  pinMode(FANSENSOR, INPUT);
  pinMode(PELTIER, OUTPUT);
  digitalWrite(FANSENSOR,HIGH);
  analogReference(EXTERNAL);
  module.clearDisplay();
  module.setDisplayToString("Tgt "+ String(tempTarget) + " C");
  delay(1000);
}
 
void loop(void) {
  uint8_t i;
  float average = 0;

  delay(500);

  /* --- Samples --- */
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(THERMISTORPIN);
   average += samples[i];
   delay(10);
  }
  average /= NUMSAMPLES;
 
  // convert the value to resistance
  average = SERIESRESISTOR / (1023 / average - 1);
  //Serial.print("Thermistor resistance "); 
  //Serial.println(average);
 
  float steinhart;                                  // Steinhart-Hart Equation
  steinhart = log(average / THERMISTORNOMINAL);     // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                        // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                      // Invert
  steinhart -= 273.15;                              // convert to C
 
  Serial.print("Temperature "); 
  Serial.print(steinhart);
  Serial.println(" C");
  module.clearDisplay();
  module.setDisplayToString(String(steinhart) + " C");

  /* --- Fan speed controller and Peltier transistor controller --- */
  /*   It forces the temperature to the range of +- 1ºC from the TargetT   */
  if(steinhart > (tempTarget + 0.3)){
    fanControl = 255;
    peltierControl = 255;
  }else if(steinhart <= (tempTarget + 0.3) && steinhart > (tempTarget - 0.3)){
    fanControl = 127;
    peltierControl = 127;
  }else{
    fanControl = 0;
    peltierControl = 0;
  }

  Serial.println("Peltier = " + String(peltierControl));
  analogWrite(FANPWM, fanControl);          // set fan speed between 0 and 255 (up to 100%)
  analogWrite(PELTIER, peltierControl);
  
  fanRPM = ((1000000/pulseIn(FANSENSOR, HIGH))/PULSE*25); //gets the frequency and coverts to RPM
  Serial.print("RPM      = ");
  Serial.println(fanRPM);
  Serial.println();
  checkButtons();
  
}

void checkButtons(){
  //module.setDisplayToDecNumber(buttons,0,false);
  switch(module.getButtons()){
    case 1: //button 1
      module.clearDisplay();
      module.setDisplayToString("Tgt "+ String(tempTarget) + " C");
    break;
    case 2: //button 2
      tempTarget--;
      module.clearDisplay();
      module.setDisplayToString("Tgt "+ String(tempTarget) + " C");
    break;
    case 4: //button 2
      tempTarget++;
      module.clearDisplay();
      module.setDisplayToString("Tgt "+ String(tempTarget) + " C");
    break;
    case 128: //button 8
      brightness++;
      if(brightness > 7){
        brightness = 1;
      }
      module.setupDisplay(true, brightness);
      module.clearDisplay();
      module.setDisplayToString("Light " + brightness);
    break;
    default:
      break;
    }
  }

