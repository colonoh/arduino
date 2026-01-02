#include <SPI.h>
#include <avr/sleep.h>

const int MEM_CHIP_SELECT_PIN = 0;  // PA4
const int LED_PIN = 6;  // PB1
const int AMP_SHUTDOWN_PIN = 7;  // PB0
const int BUTTON1_PIN = 4;  // PB3 Button to ground, uses internal pullup
const int BUTTON2_PIN = 5;  // PB2 Button to ground, uses internal pullup

volatile uint32_t samplesRemaining = 0;
volatile uint16_t avg = 0;
const uint8_t threshold = 30;
volatile bool playing = false;  // true when actively playing audio

struct Track {
  uint32_t offset;
  uint32_t length;
};

const Track tracks[] = {
  {0x000000d6, 0x00011280},  //  0: abcdelicious
  {0x00011356, 0x00050b80},  //  1: abcsingwithme
  {0x00061ed6, 0x00007b00},  //  2: bubbles
  {0x000699d6, 0x00007080},  //  3: chimesounds
  {0x00070a56, 0x00019080},  //  4: creamandsugar
  {0x00089ad6, 0x00008580},  //  5: goodjob
  {0x00092056, 0x0001ce00},  //  6: happyandyouknowit
  {0x000aee56, 0x00006000},  //  7: hyper
  {0x000b4e56, 0x0000d680},  //  8: iknewyoucouldbrewit
  {0x000c24d6, 0x0000db00},  //  9: onetwothreemoresugarplease
  {0x000cffd6, 0x00029d00},  // 10: onetwothreesingwithme
  {0x000f9cd6, 0x00005e80},  // 11: pipessound
  {0x000ffb56, 0x00035e80},  // 12: redorangeyellowgreenandblue
  {0x001359d6, 0x0000be80},  // 13: sipbleepahhh
  {0x00141856, 0x00007800},  // 14: someotherpipesthing
  {0x00149056, 0x0000f780},  // 15: thanksalatte
  {0x001587d6, 0x0000687e},  // 16: areyoustillthere
  {0x0015f054, 0x00009cbc},  // 17: coffeeinmycoffeehole
  {0x00168d10, 0x00009cbc},  // 18: coffeeinmycoffeehole
  {0x001729cc, 0x00009cbc},  // 19: coffeeinmycoffeehole
  {0x0017c688, 0x00009cbc},  // 20: coffeeinmycoffeehole
  {0x00186344, 0x00009cbc},  // 21: coffeeinmycoffeehole
  {0x00190000, 0x00004cbc},  // 22: getmad
  {0x00194cbc, 0x000118d1},  // 23: i_like_your_style
};
const uint8_t NUM_TRACKS = 24;

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
// triggers when ANY pin on PORTB generates an interrupt.
ISR(PORTB_PORT_vect) {
  PORTB.PIN3CTRL &= ~PORT_ISC_gm;  // clears the interrupt sense configuration bits for pin 3, effectively disabling further interrupts on that pin.
  PORTB.PIN2CTRL &= ~PORT_ISC_gm;  // clears the interrupt sense configuration bits for pin 2
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
    if (i == BUTTON1_PIN || i == BUTTON2_PIN) continue;  // Don't touch button pins
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  // keeps flash CS high (deselected) so the flash stays in deep power-down mode and doesn't interpret noise as commands
  digitalWrite(MEM_CHIP_SELECT_PIN, HIGH);

  VPORTB.INTFLAGS = (1 << 3) | (1 << 2);  // clears any pending interrupt flags on PB3 and PB2 before enabling the interrupts.
  PORTB.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_LEVEL_gc;
  PORTB.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_LEVEL_gc;

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sei();
  sleep_cpu();
  sleep_disable();

  delay(50);  // Debounce
  SPI.begin();
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(MEM_CHIP_SELECT_PIN, OUTPUT);
  pinMode(AMP_SHUTDOWN_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  digitalWrite(MEM_CHIP_SELECT_PIN, HIGH);
  digitalWrite(AMP_SHUTDOWN_PIN, LOW);

  SPI.begin();

  // Setup TCB0 for 16kHz interrupt (10MHz / 625 = 16kHz)
  TCB0.CCMP = 624;
  TCB0.INTCTRL = TCB_CAPT_bm;  // Enables the capture interrupt, which fires when the counter reaches CCMP.
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc;  // Sets clock source to CLK_PER with no prescaler (divide by 1).

  // play the intro song
  playAudio(0x001a658d, 0x00056398); 
}

void loop() {
  goToSleep();
  if (digitalRead(BUTTON1_PIN) == LOW || digitalRead(BUTTON2_PIN) == LOW) {
    randomSeed(micros());
    uint8_t idx = random(NUM_TRACKS);
    playAudio(tracks[idx].offset, tracks[idx].length);
  }
}
