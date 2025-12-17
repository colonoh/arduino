#include <SPI.h>

const int CS_PIN = 0;  // Chip select - adjust for your wiring
const int LED_PIN = 5;  // LED pin - adjust for your wiring

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  SPI.begin();
  delay(100);

  // Read JEDEC ID (command 0x9F)
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x9F);
  uint8_t manufacturer = SPI.transfer(0x00);
  digitalWrite(CS_PIN, HIGH);

  // W25Q128 manufacturer ID is 0xEF (Winbond)
  if (manufacturer == 0xEF) {
    // Success: 5 fast blinks
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  } else {
    // Failure: LED stays on
    digitalWrite(LED_PIN, HIGH);
  }
}

void loop() {
}
