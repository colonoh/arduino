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

// triggers when Timer/Counter B0 fires, every 1/16,000th of a second
ISR(TCB0_INT_vect) {
  TCB0.INTFLAGS = TCB_CAPT_bm;  // clears the interrupt flag
  if (samplesRemaining > 0) {
    uint8_t sample = SPI.transfer(0x00);  // get a byte from the memory chip
    DAC0.DATA = sample;

    // turn on/off the LED so it looks like an audio VU meter
    uint8_t amplitude = (sample > 128) ? (sample - 128) : (128 - sample);  // calculates the absolute deviation from center (128 is silence in unsigned 8-bit audio)
    avg = ((avg * 7) + (amplitude * 8)) >> 3;
    digitalWrite(LED_PIN, avg > threshold);
    samplesRemaining--;
  }
  else 
  {
    playing = false;  // we're done playing
  } 
}

// button interrupt for wake from sleep
// triggers when any pin on PORTB generates an interrupt.
ISR(PORTB_PORT_vect) {
  PORTB.PIN3CTRL &= ~PORT_ISC_gm;  // clears the interrupt sense configuration bits for pin 3, effectively disabling further interrupts on that pin.
  // clears all pending interrupt flags on PORTB by writing the flags back to themselves (writing a 1 clears the flag)
  // this acknowledges the interrupt that woke the MCU
  VPORTB.INTFLAGS = VPORTB.INTFLAGS;
}

void playAudio(uint32_t startAddr, uint32_t numSamples) {
  // Wake flash from deep power-down
  digitalWrite(MEM_CHIP_SELECT_PIN, LOW);
  SPI.transfer(0xAB);  // send the Release from Deep Power-Down command
  digitalWrite(MEM_CHIP_SELECT_PIN, HIGH);
  delayMicroseconds(5);

  // Enable DAC and amp
  DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;  // Enables the DAC and its output pin.
  DAC0.DATA = 128;  // 128 = silence
  delay(10);
  digitalWrite(AMP_SHUTDOWN_PIN, HIGH);

  // Start flash read
  digitalWrite(MEM_CHIP_SELECT_PIN, LOW);
  SPI.transfer(0x03);  // send "Read Data" command
  SPI.transfer((startAddr >> 16) & 0xFF);  // first 8 bits of the 24-bit start address, MSB first
  SPI.transfer((startAddr >> 8) & 0xFF);
  SPI.transfer(startAddr & 0xFF);

  samplesRemaining = numSamples;
  playing = true;
  TCB0.CTRLA |= TCB_ENABLE_bm;  // Enables the timer, which starts firing the 16kHz ISR
  while (playing);  // will block here until playing == False
  TCB0.CTRLA &= ~TCB_ENABLE_bm;  // Disables the timer to stop the ISR

  digitalWrite(MEM_CHIP_SELECT_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);
  DAC0.DATA = 128;

  // Shutdown amp and flash
  digitalWrite(AMP_SHUTDOWN_PIN, LOW);
  digitalWrite(MEM_CHIP_SELECT_PIN, LOW);
  SPI.transfer(0xB9);  // sends "Deep Power-Down" command
  digitalWrite(MEM_CHIP_SELECT_PIN, HIGH);
  DAC0.CTRLA = 0;
}

void goToSleep() {
  SPI.end();

  // set (almost) all the pins to OUTPUT mode and send LOW to save power
  for (uint8_t i = 0; i < 11; i++) {
    if (i == BUTTON1_PIN) continue;  // Don't touch button pin
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  // keeps flash CS high (deselected) so the flash stays in deep power-down mode and doesn't interpret noise as commands
  digitalWrite(MEM_CHIP_SELECT_PIN, HIGH);

  pinMode(BUTTON1_PIN, INPUT_PULLUP);  // Sets button pin as input with internal pull-up resistor.
  VPORTB.INTFLAGS = (1 << 3);  // clears any pending interrupt flag on PB3 (bit 3) before enabling the interrupt.
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
  TCB0.INTCTRL = TCB_CAPT_bm;  // Enables the capture interrupt, which fires when the counter reaches CCMP.
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc;  // Sets clock source to CLK_PER with no prescaler (divide by 1).
}

void loop() {
  goToSleep();
  SPI.begin();
  playAudio(0x0017f27d, 16000UL * 22);
}
