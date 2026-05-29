/*
 * Thermogenesis v0.1.0 — Breadboard Prototype Firmware
 * https://github.com/SargonLabs/Thermogenesis
 *
 * ESP32-S3 + MLX90614 thermopile + capacitive touch + SK6812 RGBW LEDs
 * Conduction-based thermal sensing through glass enclosure
 * evidence_grade classification: measured / estimated / symbolic
 *
 * Licence: GPL-3.0
 * Author: William Janczewski / Sargon Labs
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_NeoPixel.h>

#define PIN_SDA       8
#define PIN_SCL       9
#define PIN_TOUCH     4
#define PIN_LED_DATA  18
#define LED_COUNT     15
#define TOUCH_THRESHOLD 27000
#define TEMP_THRESHOLD 0.5

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_NeoPixel leds(LED_COUNT, PIN_LED_DATA, NEO_GRBW + NEO_KHZ800);

float ledWarmth[LED_COUNT];
float targetWarmth = 0.0;
unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("--- Thermogenesis v0.1.0 ---");

  Wire.setPins(PIN_SDA, PIN_SCL);

  if (!mlx.begin()) {
    Serial.println("ERROR: MLX90614 not found. Check wiring.");
    while (1) delay(1000);
  }
  Serial.println("MLX90614 connected.");

  leds.begin();
  leds.setBrightness(50);

  for (int i = 0; i < LED_COUNT; i++) {
    ledWarmth[i] = 0.0;
  }

  Serial.println("Ready.");
  Serial.println();
}

void setLedFromWarmth(int i, float w, float pulse) {
  uint8_t r, g, b, wh;

  if (w < 0.05) {
    float brightness = 0.2 + 0.12 * pulse;
    r = 0;
    g = 0;
    b = (uint8_t)(70 * brightness);
    wh = 0;
  } else if (w < 0.25) {
    float t = (w - 0.05) / 0.2;
    float brightness = 0.4 + 0.1 * pulse;
    r = (uint8_t)(20 * t * brightness);
    g = 0;
    b = (uint8_t)((70 + 30 * t) * brightness);
    wh = 0;
  } else if (w < 0.5) {
    float t = (w - 0.25) / 0.25;
    r = (uint8_t)(20 + 150 * t);
    g = (uint8_t)(40 * t);
    b = (uint8_t)(100 * (1.0 - t));
    wh = (uint8_t)(30 * t);
  } else if (w < 0.75) {
    float t = (w - 0.5) / 0.25;
    r = (uint8_t)(170 + 50 * t);
    g = (uint8_t)(40 + 20 * t);
    b = 0;
    wh = (uint8_t)(30 + 40 * t);
  } else {
    float brightness = 0.8 + 0.2 * pulse;
    r = (uint8_t)(230 * brightness);
    g = (uint8_t)(70 * brightness);
    b = 0;
    wh = (uint8_t)(80 * brightness);
  }

  leds.setPixelColor(i, leds.Color(r, g, b, wh));
}

void loop() {
  float ambient = mlx.readAmbientTempC();
  float object  = mlx.readObjectTempC();
  float delta   = object - ambient;
  int   touchRaw = touchRead(PIN_TOUCH);
  bool  touched  = (touchRaw > TOUCH_THRESHOLD);

  if (delta < 0.1) {
    targetWarmth = 0.0;
  } else if (delta < 2.5) {
    targetWarmth = delta / 2.5;
  } else {
    targetWarmth = 1.0;
  }

  float time = millis() / 1000.0;

  for (int i = 0; i < LED_COUNT; i++) {
    float offset = (float)i / (float)LED_COUNT;

    float delay_factor = offset * 0.4;
    float localTarget = targetWarmth - delay_factor;
    if (localTarget < 0) localTarget = 0;
    if (localTarget > 1) localTarget = 1;

    if (localTarget > ledWarmth[i]) {
      ledWarmth[i] += 0.015 + 0.008 * (1.0 - offset);
      if (ledWarmth[i] > localTarget) ledWarmth[i] = localTarget;
    } else {
      ledWarmth[i] -= 0.06;
      if (ledWarmth[i] < localTarget) ledWarmth[i] = localTarget;
      if (ledWarmth[i] < 0) ledWarmth[i] = 0;
    }

    float localPulse = sin(time * 0.8 + i * 0.5) * 0.5 + 0.5;

    setLedFromWarmth(i, ledWarmth[i], localPulse);
  }

  leds.show();

  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();

    Serial.print(ambient, 1);
    Serial.print("C    ");
    Serial.print(object, 1);
    Serial.print("C    ");
    Serial.print(delta, 2);
    Serial.print("C    ");
    Serial.print(touchRaw);
    Serial.print("  [");
    Serial.print(touched ? "TOUCH" : "-----");
    Serial.print("]  ");

    if (touched && delta >= TEMP_THRESHOLD) {
      Serial.println("MEASURED");
    } else if (touched) {
      Serial.println("PENDING");
    } else if (delta >= TEMP_THRESHOLD) {
      Serial.println("ESTIMATED");
    } else {
      Serial.println("IDLE");
    }
  }

  delay(30);
}
