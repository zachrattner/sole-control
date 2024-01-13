/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/**
 * This project has been modified from the following example:
 * https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/libraries/Bluefruit52Lib/examples/Peripheral/blehid_keyboard/blehid_keyboard.ino
 */

#include <bluefruit.h>

#define TRUE  1
#define FALSE 0

/** 
 * HID key code: 
 * https://gist.github.com/MightyPork/6da26e382a7ad91b5496ee55fdc73db2
 */
// Space Key
#define KEY_NEXT 32  

 // Left Arrow
#define KEY_BACK 0x50

/**
 * Pin number the voltage divider network is connected to
 * Note that this must be an analog pin.
 */
#define SENSOR_PIN 2

// Key press duration when sending key press signal over the Bluteooth interface
#define KEY_PRESS_DURATION_MS 5

// Sensor durations (in ms) for a short (next) and long (back) key press
#define PRESS_DURATION_SHORT_MS 500
#define PRESS_DURATION_LONG_MS  2000

// ADC readings run from  0 (0 V) to 1023 (3.3 V) 
#define SENSOR_PRESS_THRESHOLD   400 
#define SENSOR_RELEASE_THRESHOLD 50  

#define SERIAL_BAUD_RATE 115200

// Pause between main loop iterations
#define LOOP_DELAY_MS 100

BLEDis         ble_dis;       // Bluetooth Device Information Service helper
BLEHidAdafruit ble_hid;       // Bluetooth Human Interface Device (HID) helper

bool     was_pressed = FALSE; // Bookkeeping variable to track rising/falling edge transitions
uint32_t press_ts    = 0;     // Timestamp of the last press
bool     debug_mode = TRUE;   // Whether to print debug lines over serial
bool     is_verbose = FALSE;  // Applies only in debug mode, prints every ADC reading

void setup() {
  pinMode(SENSOR_PIN, INPUT);
  
  Serial.begin(SERIAL_BAUD_RATE);

  // Wait for serial port to become active
  while (!Serial) { 
    delay(10); // for nrf52840 with native usb
  }

  Serial.println("Toemote booting up");

  Bluefruit.begin();
  Bluefruit.setTxPower(8); // Check bluefruit.h for supported values

  // Configure and Start Device Information Service
  ble_dis.setManufacturer("Worthwhile Adventures LLC");
  ble_dis.setModel("Toemote");
  ble_dis.begin();

  /* Start BLE HID
   * Note: Apple requires BLE device must have min connection interval >= 20m
   * ( The smaller the connection interval the faster we could send data).
   * However for HID and MIDI device, Apple could accept min connection interval 
   * up to 11.25 ms. Therefore BLEHidAdafruit::begin() will try to set the min and max
   * connection interval to 11.25  ms and 15 ms respectively for best performance.
   */
  ble_hid.begin();

  /* Set connection interval (min, max) to your perferred value.
   * Note: It is already set by BLEHidAdafruit::begin() to 11.25ms - 15ms
   * min = 9*1.25=11.25 ms, max = 12*1.25= 15 ms 
   */
  /* Bluefruit.Periph.setConnInterval(9, 12); */

  // Set up and start advertising
  startAdv();

  Serial.println("Toemote running...");
}

void startAdv(void) {  
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);
  
  // Include BLE HID service
  Bluefruit.Advertising.addService(ble_hid);

  // There is enough room for the dev name in the advertising packet
  Bluefruit.Advertising.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void loop() {  
  char     pressed_key  = 0;
  uint32_t sensor_value = analogRead(SENSOR_PIN);
  bool     is_pressed   = sensor_value >= SENSOR_PRESS_THRESHOLD;
  bool     is_released  = sensor_value < SENSOR_RELEASE_THRESHOLD;
  uint32_t now          = millis();

  if (debug_mode && is_verbose) {
    Serial.print("[");
    Serial.print(now);
    Serial.print("] ADC: ");
    Serial.print(sensor_value);
    Serial.print(", Was Pressed: ");
    Serial.print(was_pressed);
    Serial.print(", Is Pressed: ");
    Serial.print(is_pressed);
    Serial.print(", Is Released: ");
    Serial.println(is_released);
  }

  // Log start time if the sensor got pressed
  if (!was_pressed && is_pressed) {
    press_ts = now;

    if (debug_mode) {
      Serial.print("Sensor press detected @ ts ");
      Serial.println(press_ts);
    }

    was_pressed = TRUE;
  }
  // Register key release
  else if (was_pressed && is_released) {
    uint32_t press_duration = millis() - press_ts;
    if (debug_mode) {
      Serial.print("Release detected, duration: ");
      Serial.println(press_duration);
    }

    // Long press goes back
    if (press_duration >= PRESS_DURATION_LONG_MS) {
      pressed_key = KEY_BACK;
    }
    // Short press goes forward
    else if (press_duration >= PRESS_DURATION_SHORT_MS) {
      pressed_key = KEY_NEXT;
    }

    was_pressed = FALSE;
  }

  // Handle key press if present
  if (pressed_key) {
      if (debug_mode) {
        Serial.print("Sending key: ");

        if (pressed_key == KEY_NEXT) {
          Serial.println("NEXT");
        }
        else if (pressed_key == KEY_BACK) {
          Serial.println("BACK");
        }
      }

    ble_hid.keyPress(pressed_key);
    delay(KEY_PRESS_DURATION_MS);

    ble_hid.keyRelease();
    delay(KEY_PRESS_DURATION_MS);
  }

  delay(LOOP_DELAY_MS);
}

