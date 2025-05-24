/*
  Written by KD9RCA, some code used from VE6BTC and WT4Y
  Written in IDE 1.8.13
  Update #defines in VARIABlES section below to desired settings

  The purpose of this sketch is to use your Baofeng UV5R or other radio
  to tx morse code for a fox hunt.

  DTMF commands are listed on line 112
  Make sure radio volume is turned up - About 1/4 turn works on a Baofeng UV-5R, it can go too high and too low

  Cycle runs when powered on, and listens for DTMF commands whenever not transmitting
*/
#include <DTMF.h> //includes DTMF library, included as .zip file

//######VARIABLES######//
#define morseCodeHz 600           //the approximate frequency of the tones in hz, in reality it will be a tad lower, and includes lots of harmonics, default 600
#define dit 64                    //the length of the dit in milliseconds. The dah and pauses are derived from this, default 64
#define rest 38000                //the amount of time between transmits in ms, default 30000 = 30seconds
#define tx 7                      //the pin on the board controls the ptt on radio, signal to 5v relay where you would have speaker/mic pins connected, default 7
#define jumperInputPin 10         //Pin to use for the jumper input
#define audio 5                   //the pin on the board that outputs audio to the radio, connected to mic on radio, default 5
#define audioIn A0                //the pin on the board that receives audio from the radio for DTMF, connected to speaker out on radio, default A0
#define ledPin 13                 //Pin to toggle when transmitting a tone for a visual reference
#define patternDelay 20           //time in ms, spacing of individual tones in pattern, default 20
#define toneDuration 50           //time in ms, length of individual tone in pattern, default 50
#define DTMFacknowledgeDelay 1400 //time in ms, time between receivng DTMF command (tone) and transmitting "R" morse code acknowledgement, default 2000
#define DTMFtxDelay 100           //time in ms, time between enabling ptt to when DTMF acknowledgement starts
#define patternCount 2            //number of times to repeat tone pattern at beginning of cycle, default 4
#define batteryVoltagePin A2      //the pin on the board the receives a voltage from the power source, make sure input voltage is between 0V and 5V, default A2
#define voltageMultiplier 1       //multiplier of calculated voltage, default 1, default setup with a 2 cell lipo battery with a 1:2 voltage divider, reads out voltage for average of 1 cell
#define voltageAudioOffset -200   //offset of morsecode played for voltage reading in hz, default -200
#define lowVoltageCutoff 3.4      //voltage at which radio stops the normal transmit cycle, default 3.4


//CW array DO NOT CHANGE
// 1 = dit , 2 = dah, 0 = space
// Letters/#                 A     B      C     D    E    F      G     H      I     J     K      L    M     N    O     P      Q      R     S    T    U      V     W      X     Y      Z       0        1      2       3       4       5       6       7       8       9
String morseTable[] = {"0", "12", "2111", "2121", "211", "1", "1121", "221", "1111", "11", "1222", "212", "1211", "22", "21", "222", "1221", "2212", "121", "111", "2", "112", "1112", "122", "2112", "2122", "2211", "22222", "12222", "11222", "11122", "11112", "11111", "21111", "22111", "22211", "22221"};



// Declaring Function prototypes
String formMorse(String input);
void playcode(String input);
void playtone(int note_duration);


// Change to your call sign and text you want to play
String textNormal = "KD9RCA FOX SSSSSS";                    //This is what will be transmitted normally, KD9RCA FOX SSSSSS
String textNormal_morse = formMorse(textNormal);
String textAlternate = "KD9RCA FOX OOOOOO";                 //This will be transmitted instead if pin 10 is held high between cycles, KD9RCA FOX OOOOOO
String textAlternate_morse = formMorse(textAlternate);


// Global Variables
bool status = 0;
int TextChars = 15;
int CodeChars;
int duration;
char thischar;
unsigned long timeToPlay = 0;
bool run = true;
bool altSet = 0;
float batteryVoltage = 0;

float n = 128.0;
float sampling_rate = 8926.0;
DTMF dtmf = DTMF(n, sampling_rate);
float d_mags[8];

//Pattern sequence, default is derived from Byonics Micro-Fox pattern
int pattern[] = {1, 5, 1, 5, 1, 3, 3, 3, 1, 5, 1, 5, 1, 3, 3, 3, 1, 6, 1, 6, 1, 4, 4, 4, 1, 6, 1, 6, 1, 4, 4, 4, 1, 7, 1, 7, 1, 5, 5, 5, 1, 7, 1, 7, 1, 5, 5, 5, 1, 8, 1, 8, 1, 6, 6, 6, 1, 8, 1, 8, 1, 6, 6, 6};




//###### SETUP AND LOOP ######//
void setup() {                        //sets the pins to desired mode
  pinMode(tx, OUTPUT);
  pinMode(jumperInputPin, INPUT_PULLUP);         //Jumper input pin
  pinMode(audio, OUTPUT);
  pinMode(ledPin, OUTPUT);               //used to see how the cw looks with onboard led pin 13, comment entire line to disable
  pinMode(batteryVoltagePin, INPUT);                //Voltage measurement pin
  status = digitalRead(10);
  Serial.begin(115200);
  
  /* Used for Debugging, don't turn all on at once
  
  Serial.println("Alternative text status: "+status);
  Serial.println("Text to be played normally: "+textNormal);
  Serial.println("Converted text to be played normally: "+textNormal_morse);
  Serial.println("Text to be played alternately: "+textAlternate);
  Serial.println("Converted text to be played alternately: "+textAlternate_morse);
  */
} // End of setup loop


void loop() {
  if (run == true && timeToPlay <= millis()) {
    digitalWrite(tx, HIGH);                         //starts the radio transmitting

    for (int i = 0; i < patternCount; i++) {                   //Plays tone pattern 3 times
      playPattern();
    }
    delay(1000);

    //status = digitalRead(10);                       //Updates status variable-USED
    Serial.println(status);
    if (altSet){                                    //Chooses alternate or normal text based on altSet
      //Serial.println(textAlternate_morse);
      
      playcode(textAlternate_morse, morseCodeHz);   //Plays found morse
    } // End of if status statment
    else {
      //Serial.println(textNormal_morse);
      
      playcode(textNormal_morse, morseCodeHz);      //Plays normal (not found) morse
    } // End of Else statment

    digitalWrite(tx, LOW);                          //Stops the radio's transmission
    timeToPlay = millis()+rest;
    readVoltage()

    //delay(rest);                                  //Delay in transmissions, deprecated
  } // End of run loop
  checkDTMF();                                      //Calls DTMF function
} // End of loop

//######DTMF######//
/*
   1- Start Fox
   2- Stop Fox
   4- Play normal text
   5- Play alternate text
   7- Play battery voltage
*/

void checkDTMF() {
    dtmf.sample(audioIn);
    dtmf.detect(d_mags, 506);
    thischar = dtmf.button(d_mags, 1800.);
    
    if (thischar) {                               // decide what to do if DTMF tone is received
      switch (thischar) {
        case 49:  //DTMF number 1, ascii
          delay(DTMFacknowledgeDelay);
          digitalWrite(tx, HIGH);                 //Starts radio transmitting
          delay(DTMFtxDelay);                     //Delays for relay to have time to switch
          playcode(formMorse("D"), morseCodeHz);  //Plays the dah-dit-dit to acknowledge command
          digitalWrite(tx, LOW);                  //Stops radio transmitting
          run = true;                             //Sets the flag to enable transmissions
          timeToPlay = millis();
          break;
        case 50:  //DTMF number 2, ascii
          delay(DTMFacknowledgeDelay);
          digitalWrite(tx, HIGH);                 //Starts radio transmitting
          delay(DTMFtxDelay);                     //Delays for relay to have time to switch
          playcode(formMorse("DD"), morseCodeHz); //Plays the dah-dit-dit twice to acknowledge command
          digitalWrite(tx, LOW);                  //Stops radio transmitting
          run = false;                            //Sets the flag to disable transmissions
          break;
        case 52:  //DTMF number 4, ascii
          delay(DTMFacknowledgeDelay);
          digitalWrite(tx, HIGH);                 //Starts radio transmitting
          delay(DTMFtxDelay);                     //Delays for relay to have time to switch
          playcode(formMorse("EEEE"), morseCodeHz); //Plays the dit 4 times to acknowledge command
          digitalWrite(tx, LOW);                  //Stops radio transmitting
          altSet = 0;                             //Sets the flag to use normal text
          break;
        case 53:  //DTMF number 5, ascii
          delay(DTMFacknowledgeDelay);
          digitalWrite(tx, HIGH);                 //Starts radio transmitting
          delay(DTMFtxDelay);                     //Delays for relay to have time to switch
          playcode(formMorse("EEEEE"), morseCodeHz); //Plays the dit 5 times to acknowledge command
          digitalWrite(tx, LOW);                  //Stops radio transmitting
          altSet = 1;                             //Sets the flag to use alternate text
          break;
        case 55:  //DTMF number 7, ascii
          delay(DTMFacknowledgeDelay);
          txVoltageMeasurement();                 //Runs voltage transmit function
          break;
      }
    }
  }


//######FUNCTIONS######//
void playPattern() {
  for (int i = 0; i < sizeof(pattern) / sizeof(pattern[0]); i++) {
    if (pattern[i] == 1) {
      delay(toneDuration);
    } else {
      playtone(toneDuration, (pattern[i] * 100));
    }
    delay(patternDelay);
  }
}

//   Function definition: playtone
void playtone(int note_duration, int hz) {
  int note = 1000000 / hz; //converts the frequency into period
  long elapsed_time = 0;
  long startTime = millis();
  if (note_duration > 0) {
    digitalWrite(13, HIGH);                  //See when it is making a tone on the led, comment line completely to disable
    while (elapsed_time < note_duration) {
      digitalWrite(audio, HIGH);
      delayMicroseconds(note / 2);
      digitalWrite(audio, LOW);
      delayMicroseconds(note / 2);
      elapsed_time = millis() - startTime;
    }
    digitalWrite(13, LOW);
  }
  else { //if it's a pause this will run

    delay(dit * 2);
  }
} // End of Function "playtone"


//   Function definition: playcode
void playcode(String input, int hz) {
  for (int i = 0; i < input.length(); i++) {
    Serial.print(input[i]);
    if (input[i] == '0') { //See if it's a pause
      duration = 0;
    }
    else if (input[i] == '1') { //See if it's a dit
      duration = dit;
    }
    else if (input[i] == '2') { //See if it's a dah
      duration = dit * 3;
    }
    playtone(duration, hz);
    delay(dit); //makes a pause between sounds, otherwise each letter would be continuous.
  }
  Serial.println();
} // End of Function "playcode"


//   Function definition: formMorse
String formMorse(String input) {
  input.toUpperCase();
  String output = "";
  for (int i = 0; i < input.length() ; i++) {
    if (input[i] >= 65 && input[i] <= 90)
      output = output + morseTable[input[i] - 64] + '0';
    else if (input[i] >= 48 && input[i] <= 57)
      output = output + morseTable[input[i] - 21] + '0';
    else if (input[i] == 32)
      output = output + morseTable[0];
  }
  return output;
} // End of Function "formMorse"

//   Function definition: txVoltageMeasurement
void txVoltageMeasurement(){
  readVoltage();
  int batteryVoltageOnesPlace = int(batteryVoltage);       //gets just the place before the decimal point
  int batteryVoltageDecimalPlace = int((batteryVoltage - batteryVoltageOnesPlace) * 10);        //gets just the rounded first place after the decimal point
  Serial.println(rawVoltage);
  Serial.println(batteryVoltage);
  Serial.println(batteryVoltageOnesPlace);
  Serial.println(batteryVoltageDecimalPlace);
  
  digitalWrite(tx, HIGH);                         //Starts radio transmitting
  delay(DTMFtxDelay);                             //Delays for relay to have time to switch
  playtone(dit*3, (morseCodeHz));// + voltageAudioOffset));                    //plays long tone before voltage reading
  delay(dit * 3);
  for (int i=0; i<batteryVoltageOnesPlace; i++){
    playtone(dit * 2, (morseCodeHz + voltageAudioOffset));
    delay(dit * 2);
  }
  delay(dit * 3);
  for (int i=0; i<batteryVoltageDecimalPlace; i++){
    playtone(dit * 2, (morseCodeHz + voltageAudioOffset));
    delay(dit * 2);
  }
  delay(dit * 3);
  //playcode(formMorse(voltageMorse), (morseCodeHz+voltageAudioOffset))
  playtone(dit*3, (morseCodeHz));// + voltageAudioOffset));                    //plays long tone after voltage reading
  digitalWrite(tx, LOW);                          //stops radio transmitting
}

void readVoltage(){
  int rawVoltage = analogRead(batteryVoltagePin);                                 //reads raw value (0-1024) from defined pin
  batteryVoltage = ((float(rawVoltage) / 1024) * 5) * voltageMultiplier;           //converts raw value to actual voltage with a adjustable calibration
}
