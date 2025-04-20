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


// #define MY_DEBUG

#define ECHOSOUNDER_DEPTH "0.5"
#define ECHOSOUNDER_MAX_VALUE "99.9"

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
volatile uint8_t second_has_passed = 0;

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

const uint64_t S1_mask      = 0b0000000000000000110000000010111100000000000000000000000000000000;
const uint64_t S1_0         = 0b0000000000000000110000000010111000000000000000000000000000000000;
const uint64_t S1_1         = 0b0000000000000000010000000000010000000000000000000000000000000000;
const uint64_t S1_2         = 0b0000000000000000100000000010011100000000000000000000000000000000;
const uint64_t S1_3         = 0b0000000000000000110000000010010100000000000000000000000000000000;
const uint64_t S1_4         = 0b0000000000000000010000000000110100000000000000000000000000000000;
const uint64_t S1_5         = 0b0000000000000000110000000010100100000000000000000000000000000000;
const uint64_t S1_6         = 0b0000000000000000110000000010101100000000000000000000000000000000;
const uint64_t S1_7         = 0b0000000000000000010000000010010000000000000000000000000000000000;
const uint64_t S1_8         = 0b0000000000000000110000000010111100000000000000000000000000000000;
const uint64_t S1_9         = 0b0000000000000000110000000010110100000000000000000000000000000000;
const uint64_t S1[]         = {S1_0, S1_1, S1_2, S1_3, S1_4, S1_5, S1_6, S1_7, S1_8, S1_9};

const uint64_t S2_mask      = 0b0000000000000000000000000000000000000000000000000000000011111110;
const uint64_t S2_0         = 0b0000000000000000000000000000000000000000000000000000000011101110;
const uint64_t S2_1         = 0b0000000000000000000000000000000000000000000000000000000001000100;
const uint64_t S2_2         = 0b0000000000000000000000000000000000000000000000000000000010110110;
const uint64_t S2_3         = 0b0000000000000000000000000000000000000000000000000000000011010110;
const uint64_t S2_4         = 0b0000000000000000000000000000000000000000000000000000000001011100;
const uint64_t S2_5         = 0b0000000000000000000000000000000000000000000000000000000011011010;
const uint64_t S2_6         = 0b0000000000000000000000000000000000000000000000000000000011111010;
const uint64_t S2_7         = 0b0000000000000000000000000000000000000000000000000000000001000110;
const uint64_t S2_8         = 0b0000000000000000000000000000000000000000000000000000000011111110;
const uint64_t S2_9         = 0b0000000000000000000000000000000000000000000000000000000011101110;
const uint64_t S2[]         = {S2_0, S2_1, S2_2, S2_3, S2_4, S2_5, S2_6, S2_7, S2_8, S2_9};

const uint64_t S3_mask      = 0b0000000000000000000000000000000000000000000000001011111100000000;
const uint64_t S3_0         = 0b0000000000000000000000000000000000000000000000001011101100000000;
const uint64_t S3_1         = 0b0000000000000000000000000000000000000000000000000001000100000000;
const uint64_t S3_2         = 0b0000000000000000000000000000000000000000000000001001111000000000;
const uint64_t S3_3         = 0b0000000000000000000000000000000000000000000000001001011100000000;
const uint64_t S3_4         = 0b0000000000000000000000000000000000000000000000000011010100000000;
const uint64_t S3_5         = 0b0000000000000000000000000000000000000000000000001010011100000000;
const uint64_t S3_6         = 0b0000000000000000000000000000000000000000000000001010111100000000;
const uint64_t S3_7         = 0b0000000000000000000000000000000000000000000000001001000100000000;
const uint64_t S3_8         = 0b0000000000000000000000000000000000000000000000001011111100000000;
const uint64_t S3_9         = 0b0000000000000000000000000000000000000000000000001011011100000000;
const uint64_t S3[]         = {S3_0, S3_1, S3_2, S3_3, S3_4, S3_5, S3_6, S3_7, S3_8, S3_9};

const uint64_t DEPTH_mask   = 0b0000000000000000000000000000000000000000000000000000000000000001;
const uint64_t DEPTH        = 0b0000000000000000000000000000000000000000000000000000000000000001;
const uint64_t METERS_mask  = 0b0000000000000000000000000000000001000000000000000000000000000000;
const uint64_t METERS       = 0b0000000000000000000000000000000001000000000000000000000000000000;
const uint64_t FEET_mask    = 0b0000000000000000000000000000000000000000000000010000000000000000;
const uint64_t FEET         = 0b0000000000000000000000000000000000000000000000010000000000000000;
const uint64_t DECIMAL_mask = 0b0000000000000000000000000000000010000000000000000000000000000000;
const uint64_t DECIMAL      = 0b0000000000000000000000000000000010000000000000000000000000000000;


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

      #ifdef MY_DEBUG
      Serial.print("data As array: ");
      for (int c = 0; c < 8; c++) {
        print_hex_byte(data.asarray[c]);
      }
      Serial.println();

      Serial.print("data As uint64: ");
      // print64(data.asint);
      println64(&Serial, data.asint);
      // Serial.println();

      Serial.print("S1 mask: ");
      println64(&Serial, S1_mask);

      Serial.print("data & S1 mask: ");
      println64(&Serial, data.asint & S1_mask);
      #endif

      uint64_t tmp = data.asint & S1_mask;
      int8_t nr1 = -1;
      for (int c = 0; c < 10; c++) {
        if (S1[c] == tmp) {
          nr1 = c;
          break;
        }
      }

      #ifdef MY_DEBUG
      Serial.print("S1: ");
      Serial.println(nr1);
      #endif
      
      tmp = data.asint & S2_mask;
      int8_t nr2 = -1;
      for (int c = 0; c < 10; c++) {
        if (S2[c] == tmp) {
          nr2 = c;
          break;
        }
      }

      #ifdef MY_DEBUG
      Serial.print("S2: ");
      Serial.println(nr2);
      #endif

      tmp = data.asint & S3_mask;
      int8_t nr3 = -1;
      for (int c = 0; c < 10; c++) {
        if (S3[c] == tmp) {
          nr3 = c;
          break;
        }
      }

      #ifdef MY_DEBUG
      Serial.print("S3: ");
      Serial.println(nr3);
      #endif

      // check if depth indicator is on,
      // means that we have reliable reading
      if (second_has_passed && (data.asint & DEPTH_mask == DEPTH)) {
        second_has_passed = 0;
        
        float result_in_meters = 0;
        // nr1 can be blank
        if (nr1 > 0) {
          result_in_meters += nr1 * 100;
        }

        if (nr2 == -1) return;
        result_in_meters += nr2 * 10;

        if (nr3 == -1) return;
        result_in_meters += nr3;

        if (data.asint & DECIMAL_mask == DECIMAL) {
          result_in_meters = result_in_meters * 0.1;
        }

        if (data.asint & FEET_mask == FEET) {
          result_in_meters = result_in_meters * 0.3048;
        }

        // by default floats are not supperted by printf.
        // lets work around here as enabling them means changeing "boards.txt" file.
        int result_int = (int) result_in_meters;
        int result_dec = ((int) (result_in_meters * 10)) % 10;
        char nmea_sub_sentence[32];
        char nmea_full_sentence[128];
        sprintf(nmea_sub_sentence, "YXDPT,%i.%i,%s,%s", result_int, result_dec, ECHOSOUNDER_DEPTH, ECHOSOUNDER_MAX_VALUE);
        char* checksum = calculate_nmea_checksum(nmea_sub_sentence);
        sprintf(nmea_full_sentence, "$%s*%s\r\n", nmea_sub_sentence, checksum);
        Serial.print(nmea_full_sentence);
        status_decoded = 0;
      }

    } else {
      // consume wire and declare error
      while(Wire.available() > 0) {
        volatile int x = Wire.read();
      }
      status_received = 0;
    }
  }
}

char* calculate_nmea_checksum(char* sentence) {
  static char checksum_str[8];
  uint8_t checksum = 0;
  for (int c = 0; sentence[c] != 0 && c < 256; c++) {
    checksum = checksum ^ (uint8_t) sentence[c];
  }

  sprintf(checksum_str, "%02X", checksum);
  return checksum_str;
}

void TimerHandler1(void) {
  if (status_received++ > 30000) status_received = 10;
  if (status_decoded++ > 30000) status_decoded = 10;
  if (second_has_passed++ > 200) second_has_passed = 120;
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

  // visual indicator that we sucessfully uploaded our firmware.
  // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void print_hex_byte(short num) {
  char tmp[4];
  sprintf(tmp, "%02X", num);
  Serial.print(tmp);
}

size_t print64(Print* pr, uint64_t n) {
  char buf[21];
  char *str = &buf[sizeof(buf) - 1];
  *str = '\0';
  do {
    uint64_t m = n;
    n /= 10;
    *--str = m - 10*n + '0';
  } while (n);
  pr->print(str);
}

size_t print64(Print* pr, int64_t n) {
  size_t s = 0;
  if (n < 0) {
    n = -n;
    s = pr->print('-');
  }  return s + print64(pr, (uint64_t)n);
}

size_t println64(Print* pr, int64_t n) {
  return print64(pr, n) + pr->println();
}

size_t println64(Print* pr, uint64_t n) {
  return print64(pr, n) + pr->println();
}
