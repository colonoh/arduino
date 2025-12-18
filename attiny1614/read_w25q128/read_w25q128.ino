#include <SPI.h>
#include <avr/sleep.h>

const int MEM_CHIP_SELECT_PIN = 0;  // PA4
const int LED_PIN = 5;
const int AMP_SHUTDOWN_PIN = 7;  // PB0
const int BUTTON1_PIN = 4;  // PB3 Button to ground, uses internal pullup

volatile uint32_t samplesRemaining = 0;
volatile uint16_t avg = 0;
const uint8_t threshold = 30;
volatile bool playing = false;  // true when actively playing audio

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
  else 
  {
    playing = false;  // we're done playing
  } 
}

// Button interrupt for wake from sleep
ISR(PORTB_PORT_vect) {
  PORTB.PIN3CTRL &= ~PORT_ISC_gm;
  VPORTB.INTFLAGS = VPORTB.INTFLAGS;
}

void playAudio(uint32_t startAddr, uint32_t numSamples) {
  // Wake flash from deep power-down
  digitalWrite(MEM_CHIP_SELECT_PIN, LOW);
  SPI.transfer(0xAB);
  digitalWrite(MEM_CHIP_SELECT_PIN, HIGH);
  delayMicroseconds(5);

  // Enable DAC and amp
  DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
  DAC0.DATA = 128;
  delay(10);
  digitalWrite(AMP_SHUTDOWN_PIN, HIGH);

  // Start flash read
  digitalWrite(MEM_CHIP_SELECT_PIN, LOW);
  SPI.transfer(0x03);
  SPI.transfer((startAddr >> 16) & 0xFF);
  SPI.transfer((startAddr >> 8) & 0xFF);
  SPI.transfer(startAddr & 0xFF);

  samplesRemaining = numSamples;
  playing = true;
  TCB0.CTRLA |= TCB_ENABLE_bm;
  while (playing);
  TCB0.CTRLA &= ~TCB_ENABLE_bm;

  digitalWrite(MEM_CHIP_SELECT_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);
  DAC0.DATA = 128;

  // Shutdown amp and flash
  digitalWrite(AMP_SHUTDOWN_PIN, LOW);
  digitalWrite(MEM_CHIP_SELECT_PIN, LOW);
  SPI.transfer(0xB9);
  digitalWrite(MEM_CHIP_SELECT_PIN, HIGH);
  DAC0.CTRLA = 0;
}

void goToSleep() {
  SPI.end();

  for (uint8_t i = 0; i < 11; i++) {
    if (i == BUTTON1_PIN) continue;  // Don't touch button pin
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  digitalWrite(MEM_CHIP_SELECT_PIN, HIGH);

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  VPORTB.INTFLAGS = (1 << 3);
  PORTB.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_LEVEL_gc;

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sei();
  sleep_cpu();
  sleep_disable();

  delay(50);  // Debounce
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(MEM_CHIP_SELECT_PIN, OUTPUT);
  pinMode(AMP_SHUTDOWN_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  digitalWrite(MEM_CHIP_SELECT_PIN, HIGH);
  digitalWrite(AMP_SHUTDOWN_PIN, LOW);

  SPI.begin();

  // Setup TCB0 for 16kHz interrupt (10MHz / 625 = 16kHz)
  TCB0.CCMP = 624;
  TCB0.INTCTRL = TCB_CAPT_bm;
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc;
}

void loop() {
  goToSleep();
  SPI.begin();
  playAudio(0x0017f27d, 16000UL * 22);
}
