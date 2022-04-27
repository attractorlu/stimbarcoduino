// This is stimBarcoduino version 1 -*- c++ -*-
// CNTL 227 / DAW 210228


#define PIN_IN 3  // This is the pin on which stimulus markers are received
/* CAUTION: Be sure to put in a ~20k pull-down resistor between PIN_IN
   and ground to ensure reliable operation when stimulus marker are not
   used. */

#define PIN_OUT 4 // This is the pin on which bar codes are produced

#define FORCED_DELAY_MS 2500 // This is the barcode-free time after stimuli
#define PERIOD_MS 5000 // This is the time between bar codes
#define SHORT_US 5000  // This is the duration of a "short" bar
#define LONG_US 10000 // This is the duration of a "long" bar

unsigned long tlast;
unsigned long dtnext;
unsigned int barcode;

void setup() {
  pinMode(PIN_OUT, OUTPUT);
  digitalWrite(PIN_OUT, LOW);
  tlast = millis();
  dtnext = 1000;
  barcode = 1;
}

int abortabledelay(int dt_us) {
  unsigned int t0 = micros();
  unsigned int dt;
  while ((dt=micros()-t0) < dt_us) {
    if (digitalRead(PIN_IN)) {
      digitalWrite(PIN_OUT, HIGH);
      tlast = millis();
      dtnext = FORCED_DELAY_MS;
      return 1;
    }
  }
  return 0;
}

void makebarcode() {
  unsigned int bc = barcode;
  barcode += 3001;
  unsigned long t0 = micros();
  digitalWrite(PIN_OUT, HIGH);
  if (abortabledelay(LONG_US)) {
     return;
  }
  
  for (int k=0; k<8; k++) {
    digitalWrite(PIN_OUT, LOW);
    if (abortabledelay((bc&1) ? LONG_US : SHORT_US)) {
      return;
    }
    digitalWrite(PIN_OUT, HIGH);
    if (abortabledelay((bc&2) ? LONG_US : SHORT_US)) {
      return;
    }
    bc >>= 2;
  }
  digitalWrite(PIN_OUT, LOW);
}

void loop() {
  while (digitalRead(PIN_IN)) {
    digitalWrite(PIN_OUT, HIGH);
    tlast = millis();
    dtnext = FORCED_DELAY_MS;
  }
  digitalWrite(PIN_OUT, LOW);
  unsigned long tnow = millis();
  unsigned long dt = tnow - tlast;
  if (dt>dtnext) {
    tlast = tnow;
    dtnext = PERIOD_MS;
    makebarcode();
  }
}
