#include <SPI.h>
#include <avr/sleep.h>

const int CS_PIN = 0;
const int LED_PIN = 5;
const int SD_PIN = 7;

volatile uint32_t samplesRemaining = 0;
volatile uint16_t avg = 0;
const uint8_t threshold = 30;

ISR(TCB0_INT_vect) {
  TCB0.INTFLAGS = TCB_CAPT_bm;  // Clear interrupt flag

  if (samplesRemaining > 0) {
    uint8_t sample = SPI.transfer(0x00);
    DAC0.DATA = sample;

    uint8_t amplitude = (sample > 128) ? (sample - 128) : (128 - sample);
    avg = ((avg * 7) + (amplitude * 8)) >> 3;
    digitalWrite(LED_PIN, avg > threshold);

    samplesRemaining--;
  }
}

void playAudioWithLED(uint32_t startAddr, uint32_t numSamples) {
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x03);
  SPI.transfer((startAddr >> 16) & 0xFF);
  SPI.transfer((startAddr >> 8) & 0xFF);
  SPI.transfer(startAddr & 0xFF);

  samplesRemaining = numSamples;

  // Start timer
  TCB0.CTRLA |= TCB_ENABLE_bm;

  // Wait for playback to finish
  while (samplesRemaining > 0);

  // Stop timer
  TCB0.CTRLA &= ~TCB_ENABLE_bm;

  digitalWrite(CS_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);
  DAC0.DATA = 128;
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  pinMode(SD_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  digitalWrite(SD_PIN, HIGH);

  DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
  SPI.begin();

  // Setup TCB0 for 16kHz interrupt (10MHz / 625 = 16kHz)
  TCB0.CCMP = 624;  // Period - 1
  TCB0.INTCTRL = TCB_CAPT_bm;  // Enable interrupt
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc;  // Clock source, not enabled yet

  delay(100);

  playAudioWithLED(0x0015f034, 16000UL * 2.5);
  digitalWrite(SD_PIN, LOW);

  // Disable peripherals before sleep
  DAC0.CTRLA = 0;
  SPI.end();

  // Enter power-down sleep
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
}

void loop() {
}
