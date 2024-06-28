 /*
  * Signal LED:
  *   Nothing received from I2C: solid red.
  *   Something received from I2C, but can not decode: blinking red.
  *   Received and decoded to nmea0183: solid green.
  *   Received and outputed as debug data: solid blue.
  *
  * Debug jumper:
  *   Open: normal mode.
  *   Shorted: debug mode.
  */

#include <Wire.h>

#define TIMER_INTERRUPT_DEBUG      0
#define USE_TIMER_1     true
#define USE_TIMER_2     false
#define USE_TIMER_3     false
#define USE_TIMER_4     false
#define USE_TIMER_5     false
#include "TimerInterrupt.h"

uint16_t debug_counter = 0;
volatile uint16_t status_received = 10;
volatile uint16_t status_decoded = 10;

#define LED_OFF 0
#define LED_RED 5
#define LED_GREEN 6
#define LED_BLUE 7
#define DEBUG_PIN 4
#define BLINK true
#define NO_BLINK false

bool blink_led = 0;
bool blink_cycle = 0;
uint8_t current_led = 0;


typedef struct {
  uint8_t dummy01 : 1;
  
} fields_t;

typedef struct {
  uint8_t byte0;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3;
  uint8_t byte4;
  uint8_t byte5;
  uint8_t byte6;
  uint8_t byte7;
} rawbytes_t;

typedef union {
  uint8_t asarray[8];
  uint64_t asint = 0;
} data_t;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(DEBUG_PIN, INPUT_PULLUP);
  
  Serial.begin(9600);

  Wire.begin(0x3e);
  Wire.onReceive(receiveEvent);

  ITimer1.init();
  ITimer1.attachInterruptInterval(1000, TimerHandler1);
}

void loop() {
  if (status_received > 3) {
    set_signal_led(LED_RED, NO_BLINK);
  } else if (status_decoded > 3) {
    set_signal_led(LED_RED, BLINK);
  } else {
    if (digitalRead(DEBUG_PIN) == HIGH) {
      set_signal_led(LED_GREEN, NO_BLINK);
    } else {
      set_signal_led(LED_BLUE, NO_BLINK);
    }
  }

  do_blinking();
  
  delay(500);
}

void receiveEvent(int howMany) {
  if (digitalRead(DEBUG_PIN) == LOW) {
    // write debug info and hex bytes to usb output
    Serial.print("C: ");
    Serial.print(debug_counter++);

    Serial.print(" A: ");
    Serial.print(Wire.available());

    Serial.print(" H: ");
    Serial.print(howMany);
    Serial.print(" MSG: ");

    while(Wire.available() > 0) {
      uint8_t x = Wire.read();
      print_hex_byte(x);
    }
    Serial.println();
    status_received = 0;
    status_decoded = 0;
  } else {
    // recode to nmea0183 to usb output
    data_t data;
    if (howMany == 12) {
      for(int c = 0; c < howMany; c++) {
        uint8_t x = Wire.read();
        if (c >= 6) {
          data.asarray[c-6] = x;
        }
      }
      status_received = 0;

      for (int c = 0; c < 6; c++) {
        print_hex_byte(data.asarray[c]);
      }
      Serial.println();

      print64(data.asint);
      Serial.println();

    } else {
      // consume wire and declare error
      while(Wire.available() > 0) {
        volatile int x = Wire.read();
      }
      status_received = 0;
    }
  }
}

void TimerHandler1(void) {
  if (status_received++ > 30000) status_received = 10;
  if (status_decoded++ > 30000) status_decoded = 10;
}

void set_signal_led(int led, bool blink) {
  current_led = led;
  switch (led) {
    case LED_RED:
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE, LOW);
      digitalWrite(LED_RED, HIGH);
      break;
    case LED_GREEN:
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_BLUE, LOW);
      digitalWrite(LED_GREEN, HIGH);
      break;
    case LED_BLUE:
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_BLUE, HIGH);
      break;
    default:
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE, LOW);
      digitalWrite(LED_RED, LOW);
  }

  if (blink) {
    blink_led = 1;
  } else {
    blink_led = 0;
  }
}

void do_blinking() {
  // blue led does not semi-blink
  // mostly bc semiblinking was an afterthought
  // and blue led is connected to non-pwm pin.
  if (blink_led) {
    if (blink_cycle) {
      if (current_led == LED_BLUE) {
        set_signal_led(LED_OFF, blink_led);
      } else {
        analogWrite(current_led, 20);
      }
      blink_cycle = false;
    } else {
      set_signal_led(current_led, blink_led);
      blink_cycle = true;
    }
  } else {
    if (blink_cycle == false) {
      set_signal_led(current_led, blink_led);
      blink_cycle = true;
    }
  }
}

void print_hex_byte(short num) {
  char tmp[4];
  sprintf(tmp, "%02X", num);
  Serial.print(tmp);
}

size_t print64(uint64_t n) {
  uint8_t base = 16;
  char buf[3 * sizeof(uint64_t) + 1];   // "big enough" buffer.
  char *str = &buf[sizeof(buf) - 1];
  *str = '\0';
  do {
    char c = n % base;
    n /= base;
    *--str =  c + '0';
  } while(n);

  return Serial.write(str);
}
