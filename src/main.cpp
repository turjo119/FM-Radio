/*
An Arduino Sketch for an FM Radio clock with TEA5767. A rotary encorder is used 
to change  the frequency of the radio. The push button on the rotary encoder is
used to change the station. The Dot Matrix Display shows the current frequency
*/

#include <Arduino.h>
#include <Wire.h>
#include <radio.h>
#include <TEA5767.h>
#include <ezButton.h>  // the library to use for SW pin
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// Define the pins for the rotary encoder
#define CLK_PIN 2
#define DT_PIN 3
#define SW_PIN 4


// Stuff for Dot Matrix Display
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
// Defining size, and output pins
#define MAX_DEVICES 4
#define CS_PIN 5
MD_Parola ledMatrix = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);



volatile unsigned long last_time;  // for debouncing

// range of FM frequencies
const int fm_freq_default = 8800; // default FM frequency to tune into

volatile int fm_freq_current = 8800;
volatile int fm_freq_min = 8750;
volatile int fm_freq_max = 10800;
const int delta_freq = 10;
int fm_freq_previous;

char s[12]; // Character array to store the frequency

// Radio frequency presets
RADIO_FREQ preset [] = {
  8800,
  8970,
  9010,
  9070,
  9130,
  9290,
  9420,
  9450,
  9550,
  9660,
  9820,
  9880,
  10170,
  10370,
  10750
  };

uint16_t presetIndex = 0;  ///< Start at Station with index = 1

ezButton button(SW_PIN);  // create ezButton object that attach to pin 4

// Function prototypes
void ISR_encoderChange();

/// The band that will be tuned by this sketch is FM.
#define FIX_BAND RADIO_BAND_FM

/// The default station that will be tuned  by this sketch is 88.00 MHz.
#define FIX_STATION 8800

TEA5767 radio;    // Create an instance of Class for Si4703 Chip

uint8_t test1;
byte test2;


/// Setup a FM only radio configuration
/// with some debugging on the Serial port
void setup() {
  // open the Serial port
  Serial.begin(57600);

  // configure encoder pins as inputs
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  button.setDebounceTime(50);  // set debounce time to 50 milliseconds

  // use interrupt for CLK pin is enough
  // call ISR_encoderChange() when CLK pin changes from LOW to HIGH
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), ISR_encoderChange, RISING);

  Serial.println("Radio...");
  delay(200);

  // Initialize the Radio 
  radio.init();

  // Enable information to the Serial port
  radio.debugEnable();

  // HERE: adjust the frequency to a local sender
  radio.setBandFrequency(FIX_BAND, FIX_STATION); 
  radio.setVolume(2);
  radio.setMono(false);


  // Initialize the Dot Matrix Display
  ledMatrix.begin();         // initialize the object 
  ledMatrix.setIntensity(0); // set the brightness of the LED matrix display (from 0 to 15)
  ledMatrix.displayClear();  // clear led matrix display

  //Scrolling effect
  //ledMatrix.displayText(s, PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
} // setup


/// show the current chip data every 3 seconds.
void loop() {

  //Stuff for the encoder
  button.loop();  // MUST call the loop() function first
  
  if(fm_freq_previous != fm_freq_current) {
      Serial.print("FM frequency: ");
      Serial.println(fm_freq_current);
      fm_freq_previous = fm_freq_current;
      
      // More to do with Radio
      //char s[12];
      radio.formatFrequency(s, sizeof(s)); //Modified to remove the "Mhz"
      Serial.print("Station:"); 
      Serial.println(s);
      
      Serial.print("Radio:"); 
      radio.debugRadioInfo();
      
      Serial.print("Audio:"); 
      radio.debugAudioInfo();

      radio.setFrequency(fm_freq_current);
      // Display the frequency on the Dot Matrix Display
      ledMatrix.setTextAlignment(PA_CENTER);
      ledMatrix.print(s); // display text
  }

  if (button.isPressed()) {
    Serial.println("Changing the station...");
    if (presetIndex < (sizeof(preset) / sizeof(RADIO_FREQ)) - 1) {
      presetIndex++;
      radio.setFrequency(preset[presetIndex]);
    }
    // Go back to start
    else {
      presetIndex = 0;
      radio.setFrequency(preset[presetIndex]);
    }
    //Set as current frequency
    fm_freq_current = preset[presetIndex];
  }
  
  // For scroll effect
  // if (ledMatrix.displayAnimate()) {
  //   ledMatrix.displayReset();
  // }

} // loop

void ISR_encoderChange() {
  if ((millis() - last_time) < 50)  // debounce time is 50ms
    return;

  if (digitalRead(DT_PIN) == HIGH) {
    // the encoder is rotating in counter-clockwise direction => decrease the frequency
    fm_freq_current = fm_freq_current - delta_freq;
    if (fm_freq_current <= fm_freq_min) {
      fm_freq_current = fm_freq_min;
      }
  } 
  
  else {
    // the encoder is rotating in clockwise direction => increase the frequency
    fm_freq_current = fm_freq_current + delta_freq;
    if (fm_freq_current >= fm_freq_max) {
      fm_freq_current = fm_freq_max;
      }
  }

  last_time = millis();
}
// End.

