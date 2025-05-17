/***************************************************
To get started see https://wiki.dfrobot.com/DFPlayer_Mini_SKU_DFR0299
 ****************************************************/

#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

#define FPSerial Serial1

DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

const int buttonLeftPin = 5;  // the number of the pushbutton pin
const int buttonRightPin = 6;  // the number of the pushbutton pin

int buttonLeftState = 0;  // variable for reading the pushbutton status
int buttonRightState = 0;  // variable for reading the pushbutton status
int busy = 0;

int leftTrack = 1;
int rightTrack = 1;

void setup()
{
  FPSerial.begin(9600);
  Serial.begin(115200);
  
  if (!myDFPlayer.begin(FPSerial, /*isACK = */true, /*doReset = */true)) {  //Use serial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini online."));
  
  myDFPlayer.setTimeOut(500); //Set seriaal communictaion time out 500ms
  
  //----Set volume----
  myDFPlayer.volume(15);  //Set volume vlue (0~30).
  
  //----Set different EQ----
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  
  //----Set device we use SD as default----
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  
  pinMode(buttonLeftPin, INPUT);
  pinMode(buttonRightPin, INPUT);

}

void loop()
{
  buttonLeftState = digitalRead(buttonLeftPin);
  buttonRightState = digitalRead(buttonRightPin);

  if (buttonLeftState == HIGH && busy == 0) {
    myDFPlayer.play(5);
    busy = 1;
  }

    if (buttonRightState == HIGH && busy == 0) {
    myDFPlayer.play(4);
    busy = 1;
  }
  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); // shows Number:2 Play Finished! after file finishes, not if it is skipped midway
    if (myDFPlayer.readType() == DFPlayerPlayFinished) {
      Serial.println(F("No longer busy!"));
      busy = 0;  
    }
  }
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  
}

