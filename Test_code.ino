/*******************************************************************
 * Test code for Macropad V1.0, a multi-mode Macro keyboard with Arduino Pro Micro using row column matrix.
 * Heavily referenced from code by Ryan Bates at www.retrobuiltgames.com
 * Check his code for full explanations and more interesting macros (http://www.retrobuiltgames.com/the-build-page/macro-keyboard-v2-0/)
 * This is a stripped down version that to make use of the 9 keys (2 rotary encoders) for multiple switchable profiles.
 *******************************************************************
 
 ===================================================================
  Last modified on: 02 August 2022
 ===================================================================
 
 * (c) 2022 Jerrin James

**********************************************************************************************************/

#include "Keyboard.h"
// Library with a lot of the HID definitions and methods
// Can be useful to take a look at it see whats available
// https://github.com/arduino-libraries/Keyboard/blob/master/src/Keyboard.h
#include <Mouse.h> //there are some mouse move functions 
#include <Keypad.h> 
// This library is for interfacing with the 3x3 Matrix
// Can be installed from the library manager, search for "keypad"
// and install the one by Mark Stanley and Alexander Brevig
// https://playground.arduino.cc/Code/Keypad/  

const byte ROWS = 3; //three rows
const byte COLS = 3; //three columns

char previousPressedKey;
boolean hasReleasedKey = false;  //use for button Hold mode. Only works with one button at a time for now...

#include <Encoder.h>
//Library for simple interfacing with encoders (up to two)
Encoder RotaryEncoderA(14, 15); //the LEFT encoder (encoder A)
Encoder RotaryEncoderB(10, 16);  //the RIGHT encoder (encoder B)

#include <Wire.h> //inclusion of Adafruit's SSD1306 OLED DISPLAY Library
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO & PRO MICRO:   2(SDA),  3(SCL), ...
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin), done need tro change it necessarly
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3C for 128x64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include <Adafruit_NeoPixel.h>  //inclusion of Adafruit's NeoPixel (RBG addressable LED) library
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN            A1 // Which pin on the Arduino is connected to the NeoPixels?
#define NUMPIXELS      10 // How many NeoPixels are attached to the Arduino? 10 total, but they are address from 0,1,2,...9.

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int colorUpdate = 0;   //setting a flag to only update colors once when the mode is switched. 
const int b = 3;       // Brightness control variable. Used to divide the RBG vales set for the RGB LEDs. full range is 0-255. 255 is super bright
                       // In fact 255 is obnoxiously bright, so this use this variable to reduce the value. It also reduces the current draw on the USB

char keys[ROWS][COLS] = {
  {'1', '2', '3'},  //  the keyboard hardware is  a 3x3 grid
  {'4', '5', '6'},
  {'7', '8', '9'}, 
};
// The library will return the character inside this array when the appropriate
// button is pressed then look for that case statement. This is the key assignment lookup table.
// Layout(key/button order) looks like this
//     |---------------------|
//     |    [ 1] [ 2] [ 3]   |     * Encoder A location = key[1]      
//     |    [ 4] [ 5] [ 6]   |     * Encoder B location = Key[3]
//     |    [ 7] [ 8] [ 9]   |      NOTE: The mode button is not row/column key, it's directly wired to A0!!
//     |---------------------|
// Variables that will change:
int modePushCounter = 0;       // counter for the number of button presses
int buttonState = 0;           // current state of the button
int lastButtonState = 0;       // previous state of the button

long positionEncoderA  = -999; //encoderA LEFT position variable
long positionEncoderB  = -999; //encoderB RIGHT position variable

const int ModeButton = A0;     // the pin that the Modebutton is attached to

byte rowPins[ROWS] = {4, 5, 6 };  //connect to the row pinouts of the keypad
byte colPins[COLS] = {7, 8, 9 };  //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

int currentmodeState = 0;        // variable to make the display loop call up only when the mode button is pressed


void setup()   {                
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 

  pixels.begin();// This initializes the NeoPixel library.
  Keyboard.begin();
  pinMode(ModeButton, INPUT_PULLUP);  // initialize the button pin as a input:  
 
  lcd_welcome();

}

void loop() {
char key = keypad.getKey();
checkModeButton();

  switch (modePushCounter) {
    case 0:
    
    encoderA_Mode0();                         //custom function for encoder A
    encoderB_Mode0();                         //custom function for encoder B
    lcd_mode_0();
    //Serial.println("mode 0");
    
    
    if (key) {  //Basic windows shortcuts
      switch (key) {
      case '1': // Open Task Manager
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.press(KEY_ESC); delay(150);
      break;
        
      case '2':  //Closes active window 
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(KEY_F4); delay(150);  
      break;
        
      case '3': // Open Device Manager
      Keyboard.press(KEY_LEFT_GUI); 
      Keyboard.press('r'); delay(100); 
      Keyboard.release(KEY_LEFT_GUI); 
      Keyboard.release('r'); 
      delay(150); //give your system time to catch up with these android-speed keyboard presses
      Keyboard.println("devmgmt.msc");
      break;
        
      case '4':  //Windows Explorer
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.press('e'); delay(100); 
      break;
        
      case '5':  //Screen Snip (Win10 snipping tool)
      Keyboard.press(KEY_LEFT_GUI); 
      Keyboard.press(KEY_LEFT_SHIFT); 
      Keyboard.press('s'); delay(100);     
      break;
      
      case '6':  //Opens google chrome
      Keyboard.press(KEY_LEFT_GUI); 
      Keyboard.press('r'); 
      Keyboard.release(KEY_LEFT_GUI); 
      Keyboard.release('r');
      delay(50); 
      Keyboard.println("chrome"); 
      break;
        
      case '7':  //copy
      Keyboard.press(KEY_LEFT_CTRL); 
      Keyboard.press('c'); delay(100); 
      Keyboard.release(KEY_LEFT_GUI); 
      Keyboard.release('c'); 
      delay(150); 
      break;
        
      case '8':  // clipboard
      Keyboard.press(KEY_LEFT_GUI); 
      Keyboard.press('v'); delay(100); 
      Keyboard.release(KEY_LEFT_GUI); 
      Keyboard.release('v'); 
      delay(150);
      break;
        
      case '9':  //paste
      Keyboard.press(KEY_LEFT_CTRL); 
      Keyboard.press('v'); delay(100); 
      Keyboard.release(KEY_LEFT_GUI); 
      Keyboard.release('v'); 
      delay(150);
      break;      
      }
    delay(100); Keyboard.releaseAll();        // this releases the buttons
    }
    break;
    
    case 1:
    
    encoderA_Mode1();                         //custom function for encoder A
    encoderB_Mode1();                         //custom function for encoder A
    lcd_mode_1();
    //Serial.println("mode 1");
    
    
    if (key) {  //SolidWorks Shortcuts
      switch (key) {
      case '1': //macro example: Windows_Key+R = Run then type "calc" and press enter. Opens MS Calculator
      Keyboard.press(KEY_LEFT_GUI); 
      Keyboard.press('r'); delay(150); 
      Keyboard.release(KEY_LEFT_GUI); 
      Keyboard.release('r'); 
      delay(150);                 //give your system time to catch up with these android-speed keyboard presses
      Keyboard.println("calc");
      break; 
      
      case '2': //macro example: Opens measuring tool in Solidworks (keyboars shortcut set to Ctrl+Q in Solidworks)
      Keyboard.press(KEY_LEFT_CTRL); 
      Keyboard.press('c'); delay(100); 
      Keyboard.release(KEY_LEFT_GUI); 
      Keyboard.release('c'); 
      delay(150); 
      break;
      
      case '3': //macro example: Opens Document Properties in Solidworks (keyboars shortcut set to "Ctrl + F13" in Solidworks)
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_F13); delay(150);  //snaps window to right side of screen.
      break;
    
      case '4':  //Screen Snip (Win10 snipping tool)
      Keyboard.press(KEY_LEFT_GUI); 
      Keyboard.press(KEY_LEFT_SHIFT); 
      Keyboard.press('s'); delay(100);     
      break;
      
      case '5': //hold left shift key
      Keyboard.press(KEY_LEFT_SHIFT);
      delay(500);
      break;
      
      case '6': //macro example: Opens shortcut tool bar in Solidworks (keyboars shortcut set to "s" in Solidworks)
      Keyboard.press('s'); 
      delay(50);
      Keyboard.release('s'); 
      break;
      
      case '7': //hold left control key
      Keyboard.press(KEY_LEFT_CTRL); 
      delay(500);
      break;
      
      case '8': //macro example: Switches window in Solidworks (keyboars shortcut set to "CTRL + TAB" in Solidworks) 
      Keyboard.press(KEY_LEFT_CTRL); 
      Keyboard.press(KEY_TAB); delay(50);
      Keyboard.release(KEY_TAB);
      break; 
      
      case '9': //macro example: Exit current command in Solidworks (keyboars shortcut set to "d" in Solidworks)
      Keyboard.press('d'); 
      delay(50);
      Keyboard.release('d');
      break;  
      
    }
    delay(100); Keyboard.releaseAll(); // this releases the buttons 
  }
  break;
  }
  delay(1);
}

void checkModeButton(){
  buttonState = digitalRead(ModeButton);
  if (buttonState != lastButtonState) { // compare the buttonState to its previous state
    if (buttonState == LOW) { // if the state has changed, increment the counter
      // if the current state is LOW then the button cycled:
      modePushCounter++;
      Serial.println("pressed");
      Serial.print("number of button pushes: ");
      Serial.println(modePushCounter);
      currentmodeState = 0;
      colorUpdate = 0;      // set the color change flag ONLY when we know the mode button has been pressed. 
                            // Saves processor resources from updating the neoPixel colors all the time
    } 
    delay(50); // Delay a little bit to avoid bouncing
  }
  lastButtonState = buttonState;  // save the current state as the last state, for next time through the loop
   if (modePushCounter >1){       //reset the counter after 4 presses CHANGE THIS FOR MORE MODES
      modePushCounter = 0;}
}
 

void encoderA_Mode0(){ //testing some encoder wheel pay control for arcade games; centede, tempest...
long newPos = RotaryEncoderA.read(); 
  if (newPos != positionEncoderA && newPos > positionEncoderA) {
    positionEncoderA = newPos;
    Mouse.move(0,0,-1); //Mouse Scroll Mouse.move(x, y, wheel) range is -128 to +127
                                                               }

  if (newPos != positionEncoderA && newPos < positionEncoderA) { 
    positionEncoderA = newPos;
    Mouse.move(0,0,1); //Mouse Scroll Mouse.move(x, y, wheel) range is -128 to +127            
                          }
}

void encoderB_Mode0(){
  long newPos = RotaryEncoderB.read(); //When the encoder lands on a valley, this is an increment of 2.
  if (newPos != positionEncoderB && newPos < positionEncoderB) {
    positionEncoderB = newPos;
    Mouse.move(4,0,0);                                                           }

  if (newPos != positionEncoderB && newPos > positionEncoderB) {
    positionEncoderB = newPos;
    Mouse.move(-4,0,0);            
                                                              }
}

void encoderA_Mode1(){
  long newPos = RotaryEncoderA.read(); 
  if (newPos != positionEncoderA && newPos > positionEncoderA) {
    positionEncoderA = newPos;
    Mouse.move(0,0,-1); //Mouse Scroll Mouse.move(x, y, wheel) range is -128 to +127
                                                               }

  if (newPos != positionEncoderA && newPos < positionEncoderA) { 
    positionEncoderA = newPos;
    Mouse.move(0,0,1); //Mouse Scroll Mouse.move(x, y, wheel) range is -128 to +127            
                          }
}

void encoderB_Mode1(){
  long newPos = RotaryEncoderB.read();
  if (newPos != positionEncoderB && newPos > positionEncoderB) {
    positionEncoderB = newPos;
    Keyboard.write(KEY_RIGHT_ARROW);
                                                               }

  if (newPos != positionEncoderB && newPos < positionEncoderB) {
    positionEncoderB = newPos;
    Keyboard.write(KEY_LEFT_ARROW);
                       
                                                              }
}

void lcd_welcome(){
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20,10);
  display.println(F("Welcome"));
  display.display();
  delay(800);
}

void lcd_mode_0(){  
  if (currentmodeState == 0){
  display.clearDisplay(); 
  display.fillRect(0, 0, 128, 17, WHITE);
  display.setTextSize(2); display.setCursor(22,1);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.println(F("Windows"));

  Text("TskMgr", 5, 25, 1, false); Text("CLOSE", 48, 25, 1, false); Text("DevMgr", 90, 25, 1, false);
  Text("Explr", 5, 38, 1, false); Text("SNIP", 48, 38, 1, false); Text("CHROME", 90, 38, 1, false);
  Text("COPY", 5, 51, 1, false); Text("ClpBrd", 48, 51, 1, false); Text("PASTE", 90, 51, 1, true);
  display.display();
  Serial.println("display updtae");
  currentmodeState++;
  }
}

void lcd_mode_1() {
  if (currentmodeState == 0){
  display.clearDisplay(); 
  display.fillRect(0, 0, 128, 17, WHITE);
  display.setTextSize(2); display.setCursor(5,1);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.println(F("SolidWorks"));

  Text("CALC", 5, 25, 1, false); Text("MSURE", 48, 25, 1, false); Text("PROP", 96, 25, 1, false);
  Text("SNIP", 5, 38, 1, false); Text("SHIFT", 48, 38, 1, false); Text("S BAR", 96, 38, 1, false);
  Text("CTRL", 5, 51, 1, false); Text("S WIN", 48, 51, 1, false); Text("EXIT", 96, 51, 1, true);
  display.display();
  Serial.println("display updtae");
  currentmodeState++;
  }
}


void lcd_mode_2() {
  display.clearDisplay(); 
  display.fillRect(0, 0, 128, 17, WHITE);
  display.setTextSize(2); display.setCursor(5,1);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.println(F("Photoshop"));

  Text("CALC", 5, 25, 1, false); Text("MSURE", 48, 25, 1, false); Text("PROP", 96, 25, 1, false);
  Text("SNIP", 5, 38, 1, false); Text("SHIFT", 48, 38, 1, false); Text("S BAR", 96, 38, 1, false);
  Text("CTRL", 5, 51, 1, false); Text("S WIN", 48, 51, 1, false); Text("EXIT", 96, 51, 1, true);
  display.display();
}
void Text(String text, int x, int y, int size, boolean d) {

  display.setTextSize(size);
  display.setTextColor(WHITE);
  display.setCursor(x,y);
  display.println(text);
}
