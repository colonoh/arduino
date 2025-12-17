#include <SPI.h>

const int CS_PIN = 0;  // Chip select - adjust for your wiring
const int LED_PIN = 5;  // LED pin - adjust for your wiring

void playAudioWithLED(uint32_t startAddr, uint32_t numSamples) {
  // Start continuous read from flash
  digitalWrite(CS_PIN, LOW);
  delayMicroseconds(1);
  SPI.transfer(0x03);  // Read Data command
  SPI.transfer((startAddr >> 16) & 0xFF);
  SPI.transfer((startAddr >> 8) & 0xFF);
  SPI.transfer(startAddr & 0xFF);

  uint16_t avg = 0;
  const uint8_t threshold = 30;  // Adjust for sensitivity

  for (uint32_t i = 0; i < numSamples; i++) {
    uint8_t sample = SPI.transfer(0x00);
    DAC0.DATA = sample;  // Output to DAC

    // Convert to amplitude (distance from center 128)
    uint8_t amplitude = (sample > 128) ? (sample - 128) : (128 - sample);

    // Simple IIR low-pass filter: avg = 7/8 * avg + 1/8 * amplitude
    avg = ((avg * 7) + (amplitude * 8)) >> 3;

    // Flash LED based on filtered amplitude
    digitalWrite(LED_PIN, avg > threshold);

    // Delay for ~16kHz sample rate (62.5us per sample)
    delayMicroseconds(62);
  }

  digitalWrite(CS_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);
  DAC0.DATA = 128;  // Return to center (silence)
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;  // Enable DAC on PA6
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

  playAudioWithLED(0x0000b6, 16000UL * 5);  // 5 seconds of audio

}

void loop() {
}
