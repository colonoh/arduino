#include <SPI.h>
#include <avr/sleep.h>
#include <util/delay.h>

const int CS_PIN = 0;
const int LED_PIN = 5;
const int SD_PIN = 7;
const int BTN_PIN = 4;  // Button to ground, uses internal pullup

volatile uint32_t samplesRemaining = 0;
volatile uint16_t avg = 0;
const uint8_t threshold = 30;

ISR(TCB0_INT_vect) {
  TCB0.INTFLAGS = TCB_CAPT_bm;

  if (samplesRemaining > 0) {
    uint8_t sample = SPI.transfer(0x00);
    DAC0.DATA = sample;

    uint8_t amplitude = (sample > 128) ? (sample - 128) : (128 - sample);
    avg = ((avg * 7) + (amplitude * 8)) >> 3;
    digitalWrite(LED_PIN, avg > threshold);

    samplesRemaining--;
  }
}

// Button interrupt for wake from sleep
ISR(PORTB_PORT_vect) {
  byte flags = VPORTB.INTFLAGS;
  PORTB.PIN3CTRL &= ~PORT_ISC_gm;  // Disable interrupt
  VPORTB.INTFLAGS = flags;         // Clear flags
}

void playAudioWithLED(uint32_t startAddr, uint32_t numSamples) {
  // Wake flash from deep power-down
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0xAB);
  digitalWrite(CS_PIN, HIGH);
  delayMicroseconds(5);

  // Enable DAC and amp
  DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
  DAC0.DATA = 128;
  _delay_ms(10);
  digitalWrite(SD_PIN, HIGH);

  // Start flash read
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x03);
  SPI.transfer((startAddr >> 16) & 0xFF);
  SPI.transfer((startAddr >> 8) & 0xFF);
  SPI.transfer(startAddr & 0xFF);

  samplesRemaining = numSamples;
  TCB0.CTRLA |= TCB_ENABLE_bm;

  while (samplesRemaining > 0);

  TCB0.CTRLA &= ~TCB_ENABLE_bm;
  digitalWrite(CS_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);
  DAC0.DATA = 128;

  // Shutdown amp and flash
  digitalWrite(SD_PIN, LOW);
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0xB9);
  digitalWrite(CS_PIN, HIGH);

  DAC0.CTRLA = 0;
}

void goToSleep() {
  SPI.end();

  for (uint8_t i = 0; i < 11; i++) {
    if (i == BTN_PIN) continue;  // Don't touch button pin
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  digitalWrite(CS_PIN, HIGH);

  // Setup button for wake interrupt using LOW_LEVEL (recommended for sleep wake)
  pinMode(BTN_PIN, INPUT_PULLUP);
  VPORTB.INTFLAGS = (1 << 3);  // Clear any pending interrupt on PB3
  PORTB.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_LEVEL_gc;  // LOW level interrupt

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sei();
  sleep_cpu();
  sleep_disable();

  // Blink LED to confirm wake (using busy-wait, not delay())
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  _delay_ms(100);
  digitalWrite(LED_PIN, LOW);

  // Debounce using busy-wait
  _delay_ms(50);
}

void setup() {
  delay(1000);  // Wait for power to stabilize before doing anything

  pinMode(LED_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  pinMode(SD_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  digitalWrite(CS_PIN, HIGH);
  digitalWrite(SD_PIN, LOW);

  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);

  SPI.begin();
  sei();  // Enable global interrupts

  // Setup TCB0 for 16kHz interrupt
  TCB0.CCMP = 624;
  TCB0.INTCTRL = TCB_CAPT_bm;
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc;

  // Blink LED to show we're running
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);

  // Play once on startup
  playAudioWithLED(0x0015f034, 16000UL * 2.5);
}

void loop() {
  goToSleep();
  SPI.begin();
  playAudioWithLED(0x0015f034, 16000UL * 2.5);
}
