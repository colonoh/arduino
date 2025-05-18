/***************************************************
To get started see https://wiki.dfrobot.com/DFPlayer_Mini_SKU_DFR0299
 ****************************************************/

#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"
#include <SoftwareSerial.h>

SoftwareSerial softSerial(/*rx =*/1, /*tx =*/0);
#define FPSerial softSerial

DFRobotDFPlayerMini myDFPlayer;

const int buttonLeftPin = 3;   // the number of the pushbutton pin
const int buttonRightPin = 4;  // the number of the pushbutton pin

int buttonLeftState = 0;   // variable for reading the pushbutton status
int buttonRightState = 0;  // variable for reading the pushbutton status
int busy = 0;

int folderCounts[3];  // how many files are in folder 1/2/3

void setup() {
  FPSerial.begin(9600);

  if (!myDFPlayer.begin(FPSerial, /*isACK = */ true, /*doReset = */ true)) {  //Use serial to communicate with mp3.
    while (true)
      ;
  }
  myDFPlayer.setTimeOut(500);  //Set serial communictaion time out 500ms

  delay(500);             // seemingly necessary otherwise the volume isn't set and it's very loud
  myDFPlayer.volume(15);  //Set volume level (0~30).

  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
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

  // if the track is finished, we're free to play another track
  if (myDFPlayer.available()) {
    if (myDFPlayer.readType() == DFPlayerPlayFinished) {
      busy = 0;
      delay(500);
    }
  }
}
