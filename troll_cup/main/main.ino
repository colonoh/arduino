/***************************************************
To get started see https://wiki.dfrobot.com/DFPlayer_Mini_SKU_DFR0299
 ****************************************************/

#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

#include <SoftwareSerial.h>

SoftwareSerial softSerial(/*rx =*/10, /*tx =*/11);
#define FPSerial softSerial

DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

const int buttonLeftPin = 5;   // the number of the pushbutton pin
const int buttonRightPin = 6;  // the number of the pushbutton pin

int buttonLeftState = 0;   // variable for reading the pushbutton status
int buttonRightState = 0;  // variable for reading the pushbutton status
int busy = 0;

int folderCounts[3];  // how many files are in folder 1/2/3

void setup() {
  FPSerial.begin(9600);
  Serial.begin(115200);

  if (!myDFPlayer.begin(FPSerial, /*isACK = */ true, /*doReset = */ true)) {  //Use serial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while (true)
      ;
  }
  Serial.println(F("DFPlayer Mini online."));

  myDFPlayer.setTimeOut(500);  //Set seriaal communictaion time out 500ms

  //----Set volume----
  myDFPlayer.volume(15);  //Set volume vlue (0~30).

  //----Set different EQ----
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);

  //----Set device we use SD as default----
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);

  pinMode(buttonLeftPin, INPUT);
  pinMode(buttonRightPin, INPUT);

  // Get how many files are in each folder
  for (int i = 1; i <= 3; i++) {
    myDFPlayer.readFileCountsInFolder(i);  // seems useless but for reason I don't understand, the first call can be inaccurate
    myDFPlayer.readFileCountsInFolder(i);
    folderCounts[i - 1] = myDFPlayer.readFileCountsInFolder(i);
  }
}

void loop() {
  buttonLeftState = digitalRead(buttonLeftPin);
  buttonRightState = digitalRead(buttonRightPin);

  if (buttonLeftState == HIGH && buttonRightState == HIGH && busy == 0) {
    int track = random(1, folderCounts[2]);
    myDFPlayer.playFolder(3, track);
    busy = 1;
    delay(500);
  }

  else if (buttonLeftState == HIGH && busy == 0) {
    int track = random(1, folderCounts[0]);
    myDFPlayer.playFolder(1, track);
    busy = 1;
    delay(500);
  }

  else if (buttonRightState == HIGH && busy == 0) {
    int track = random(1, folderCounts[1]);
    myDFPlayer.playFolder(2, track);
    busy = 1;
    delay(500);
  }

  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read());  // shows Number:2 Play Finished! after file finishes, not if it is skipped midway
    if (myDFPlayer.readType() == DFPlayerPlayFinished) {
      Serial.println(F("No longer busy!"));
      busy = 0;
      delay(500);
    }
  }
}

void printDetail(uint8_t type, int value) {
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
