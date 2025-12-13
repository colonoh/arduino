// Portable W25Q128 SPI code for ATTiny85 and ATTiny1614
// ATTiny85: Uses bit-banged SPI (USI-based SPI libraries don't work reliably)
// ATTiny1614: Uses hardware SPI

#if defined(__AVR_ATtiny85__)
  // ATTiny85 - bit-bang SPI
  #define CS_PIN 3   // PB3 = physical pin 2
  #define LED_PIN 4  // PB4 = physical pin 3
  #define MOSI_PIN 0 // PB0 = physical pin 5
  #define MISO_PIN 1 // PB1 = physical pin 6
  #define SCK_PIN 2  // PB2 = physical pin 7
  #define USE_BITBANG
#else
  // ATTiny1614 - hardware SPI
  #include <SPI.h>
  #define CS_PIN 10  // Adjust for your ATTiny1614 wiring
  #define LED_PIN 7  // Adjust for your ATTiny1614 wiring
#endif

uint8_t spiTransfer(uint8_t data) {
#ifdef USE_BITBANG
  uint8_t result = 0;
  for(int i=7; i>=0; i--) {
    digitalWrite(MOSI_PIN, (data & (1<<i)) ? HIGH : LOW);
    delayMicroseconds(1);

    digitalWrite(SCK_PIN, HIGH);
    delayMicroseconds(1);

    if(digitalRead(MISO_PIN)) {
      result |= (1<<i);
    }

    digitalWrite(SCK_PIN, LOW);
    delayMicroseconds(1);
  }
  return result;
#else
  return SPI.transfer(data);
#endif
}

void spiBegin() {
#ifdef USE_BITBANG
  pinMode(CS_PIN, OUTPUT);
  pinMode(MOSI_PIN, OUTPUT);
  pinMode(MISO_PIN, INPUT);
  pinMode(SCK_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  digitalWrite(SCK_PIN, LOW);
#else
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
#endif
}

void setup() {
  pinMode(LED_PIN, OUTPUT);

  // Startup blink
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);

  spiBegin();
  delay(10);

  // Read W25Q128 JEDEC ID
  digitalWrite(CS_PIN, LOW);
  delayMicroseconds(1);
  spiTransfer(0x9F);  // Read JEDEC ID command
  uint8_t mfg = spiTransfer(0x00);
  uint8_t type = spiTransfer(0x00);
  uint8_t capacity = spiTransfer(0x00);
  delayMicroseconds(1);
  digitalWrite(CS_PIN, HIGH);

  // Expected: mfg=0xEF (Winbond), type=0x40, capacity=0x18

  // if (mfg == 0xEF) {
  // if (type == 0x40) {
  if (capacity == 0x18) {
    // Success - fast blink 10 times
    for(int i=0; i<10; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  } else {
    // Fail - slow blink forever
    while(1) {
      digitalWrite(LED_PIN, HIGH);
      delay(500);
      digitalWrite(LED_PIN, LOW);
      delay(500);
    }
  }
}

void loop() {
  // Your main code here
}
