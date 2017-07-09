/* 
  Example using a light dependent resistor (LDR) to change 
  an FM synthesis parameter, and a knob for fundamental frequency,
  using Mozzi sonification library.

  Demonstrates analog input, audio oscillators, and phase modulation.
  Also demonstrates AutoMap, which maps unpredictable inputs to a set range.
  There might be clicks in the audio from rapid control changes, which
  could be smoothed with Line or Smooth objects.
    
  This example goes with a tutorial on the Mozzi site:
  http://sensorium.github.io/Mozzi/learn/introductory-tutorial/
  
  The circuit:
     Audio output on digital pin 9 on a Uno or similar, or
    DAC/A14 on Teensy 3.1, or 
     check the README or http://sensorium.github.com/Mozzi/

     Potentiometer connected to analog pin 0.
       Center pin of the potentiometer goes to the analog pin.
       Side pins of the potentiometer go to +5V and ground
  
     Light dependent resistor (LDR) and 5.1k resistor on analog pin 1:
       LDR from analog pin to +5V (3.3V on Teensy 3.1)
       5.1k resistor from analog pin to ground
  
  Mozzi help/discussion/announcements:
  https://groups.google.com/forum/#!forum/mozzi-users

  Tim Barrass 2013, CC by-nc-sa.
*/

#include <MozziGuts.h>
#include <Oscil.h> // oscillator 
#include <tables/cos2048_int8.h> // table for Oscils to play
#include <AutoMap.h> // maps unpredictable inputs to a range
#include <CapacitiveSensor.h>
#include <RollingAverage.h>
 
// desired carrier frequency max and min, for AutoMap
const int MIN_CARRIER_FREQ = 22;
const int MAX_CARRIER_FREQ = 440;

// desired intensity max and min, for AutoMap, note they're inverted for reverse dynamics
//const int MIN_INTENSITY = 700;
//const int MAX_INTENSITY = 10;
//const int MIN_INTENSITY = 700;
//const int MAX_INTENSITY = 10;
const int MIN_INTENSITY = 700;
const int MAX_INTENSITY = 400;

const int MIN_RATIO = 500;
const int MAX_RATIO = 10;

AutoMap kMapCarrierFreq(0,1023,MIN_CARRIER_FREQ,MAX_CARRIER_FREQ);
AutoMap kMapIntensity(0,1023,MIN_INTENSITY,MAX_INTENSITY);
//AutoMap cap34Intensity(-4000, 4000, MIN_INTENSITY,MAX_INTENSITY);
AutoMap cap34Intensity(-1000, 1000, MIN_INTENSITY,MAX_INTENSITY);
AutoMap cap56Ratio(1, 10, MIN_RATIO,MAX_RATIO);

const int KNOB_PIN = 0; // set the input for the knob to analog pin 0
const int LDR_PIN = 1; // set the input for the LDR to analog pin 1

CapacitiveSensor   cs_3_4 = CapacitiveSensor(3,4);        // 10M resistor between pins 4 & 2, pin 2 is sensor pin, add a wire and or foil if desired
CapacitiveSensor   cs_5_6 = CapacitiveSensor(5,6);        // 10M resistor between pins 4 & 6, pin 6 is sensor pin, add a wire and or foil
CapacitiveSensor   cs_7_8 = CapacitiveSensor(7,8);        // 10M resistor between pins 4 & 8, pin 8 is sensor pin, add a wire and or foil
RollingAverage <int, 32> kAverage; // how_many_to_average has to be power of 2
RollingAverage <int, 8> jAverage; // how_many_to_average has to be power of 2


Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aCarrier(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aModulator(COS2048_DATA);

int mod_ratio = 10;//3;//10; // 3; // harmonics
long fm_intensity; // carries control info from updateControl() to updateAudio()
long fm_intensity_R; // carries control info from updateControl() to updateAudio()
long fm_intensity_C; // carries control info from updateControl() to updateAudio()


void setup(){
  //Serial.begin(9600); // for Teensy 3.1, beware printout can cause glitches
  Serial.begin(115200); // set up the Serial output so we can look at the piezo values // set up the Serial output for debugging
  startMozzi(); // :))
  cs_3_4.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
  cs_5_6.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
  cs_7_8.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
  
}


void updateControl(){
  // read the cap sense
  //long cs34 =  cs_3_4.capacitiveSensor(1);
//  long cs56 =  cs_5_6.capacitiveSensor(1);
//  long cs78 =  cs_7_8.capacitiveSensor(1);
  int cs34 =  int(cs_3_4.capacitiveSensor(1));
  int cs56 =  int(cs_5_6.capacitiveSensor(1));
//  int cs78 =  int(cs_7_8.capacitiveSensor(10));
  int cs34av =  kAverage.next(cs34);
  int cs56av =  jAverage.next(cs56);
//  int cs78av =  kAverage.next(cs78);

  
  // read the knob
  int knob_value = mozziAnalogRead(KNOB_PIN); // value is 0-1023

  // map the knob to carrier frequency
  int carrier_freq = kMapCarrierFreq(knob_value);
  
  //calculate the modulation frequency to stay in ratio
  int mod_freq = carrier_freq * mod_ratio;
  
  // set the FM oscillator frequencies to the calculated values
  aCarrier.setFreq(carrier_freq); 
  aModulator.setFreq(mod_freq);
  
  // read the light dependent resistor on the Analog input pin
  int light_level= mozziAnalogRead(LDR_PIN); // value is 0-1023
  
  // print the value to the Serial monitor for debugging
  //Serial.print("light_level = "); 
  //Serial.print(light_level); 
  //Serial.print("\t"); // prints a tab
  
  fm_intensity_R = kMapIntensity(light_level);
  fm_intensity_C = cap34Intensity(cs34av) ;
  
  //Serial.print("fm_intensity = ");
//  Serial.print(carrier_freq);
//  Serial.print("\t");
//  Serial.print(mod_freq);
//  Serial.print("\t");
//  Serial.print(fm_intensity);
//  Serial.print("\t");
  //Serial.println(); // print a carraige return for the next line of debugging info

  Serial.print(cs34av);
  Serial.print("\t");
//  Serial.print(cs56);
//  Serial.print("\t");
  Serial.print(fm_intensity_R);
  Serial.print("\t");
  Serial.print(fm_intensity_C);
   Serial.print("\t");
  Serial.println(cs56av);
  //Serial.println("  ");
  
  
//  Serial.print("\t");
//  Serial.print(cs78av);
//  Serial.print("\t");
  //Serial.println(); // 


}

int updateAudio(){
  long modulation = fm_intensity_C * aModulator.next(); 
  //erial.println(modulation);
  return aCarrier.phMod(modulation); // phMod does the FM
}


void loop(){
  audioHook();
}





