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
#include <bluefruit.h>

#define TRUE  1
#define FALSE 0

#define ARROW_LEFT 37  // Back ASCII code
#define ARROW_RIGHT 39 // Next ASCII code

#define SENSOR_PIN 2

#define KEY_PRESS_DURATION_MS 5

#define PRESS_DURATION_SHORT_MS 1000
#define PRESS_DURATION_LONG_MS  2000

// ADC is 0-1023 for 10 bits or 0-4095 for 12 bits
#define SENSOR_PRESS_THRESHOLD   400
#define SENSOR_RELEASE_THRESHOLD 50

BLEDis ble_dis;
BLEHidAdafruit ble_hid;

bool was_pressed = FALSE;
uint32_t press_ts;
bool debug_mode = TRUE;

void setup() 
{
  pinMode(SENSOR_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  // Start with LED red to indicate initial setup
  digitalWrite(LED_BUILTIN, LED_RED);  

  Serial.begin(115200);

  // Wait for serial port to become active
  while (!Serial) { 
    delay(10); // for nrf52840 with native usb
  }

  Serial.println("SoleControl booting up");

  Bluefruit.begin();
  Bluefruit.setTxPower(8); // Check bluefruit.h for supported values

  // Configure and Start Device Information Service
  ble_dis.setManufacturer("Worthwhile Adventures LLC");
  ble_dis.setModel("SoleControl");
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

  // Change LED to blue to indicate normal operation
  digitalWrite(LED_BUILTIN, LED_BLUE);  

  Serial.println("SoleControl running...");
}

void startAdv(void)
{  
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

void loop() 
{  
  char     pressed_key  = 0;
  uint32_t sensor_value = analogRead(SENSOR_PIN);
  bool     is_pressed   = sensor_value >= SENSOR_PRESS_THRESHOLD;
  bool     is_released  = sensor_value < SENSOR_RELEASE_THRESHOLD;
  uint32_t now          = millis();


  if (debug_mode) {
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
      pressed_key = ARROW_LEFT;
    }
    // Short press goes forward
    else if (press_duration >= PRESS_DURATION_SHORT_MS) {
      pressed_key = ARROW_RIGHT;
    }

    was_pressed = FALSE;
  }

  // Handle key press if present
  if (pressed_key) {
      if (debug_mode) {
        Serial.print("Sending key: ");

        if (pressed_key == ARROW_LEFT) {
          Serial.println("LEFT");
        }
        else if (pressed_key == ARROW_RIGHT) {
          Serial.println("RIGHT");
        }
      }

    ble_hid.keyPress(pressed_key);
    delay(KEY_PRESS_DURATION_MS);

    ble_hid.keyRelease();
    delay(KEY_PRESS_DURATION_MS);
  }

  if (debug_mode) {
    delay(100);
  }
  else {
    delay(10);
  }
}

