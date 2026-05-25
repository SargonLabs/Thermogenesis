/*
 * Thermogenesis — Breadboard Prototype Firmware
 * https://github.com/sargonlabs/thermogenesis
 *
 * Reads MLX90614 thermopile sensor (I2C), capacitive touch (GPIO),
 * classifies interactions with evidence_grade, drives SK6812 RGBW LEDs,
 * and publishes events to serial (MQTT stub included).
 *
 * Licence: GPL-3.0
 * Author: William Janczewski / Sargon Labs
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_NeoPixel.h>

// ── Pin assignments (adjust for your board) ──────────────────────
#define PIN_SDA        8
#define PIN_SCL        9
#define PIN_TOUCH      4
#define PIN_LED_DATA  18
#define LED_COUNT      5

// ── Thresholds (configurable per deployment) ─────────────────────
#define TEMP_THRESHOLD      0.5f   // °C above ambient to trigger
#define TEMP_HYSTERESIS     0.2f   // °C below threshold to reset
#define TOUCH_THRESHOLD   55000    // Raw touch value above this = touched (ESP32-S3: higher = touched)
#define CONFIRM_WINDOW_MS 10000    // Max ms between touch and thermal confirmation
#define HEARTBEAT_MS      60000    // Heartbeat interval during sustained contact

// ── State ────────────────────────────────────────────────────────
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_NeoPixel leds(LED_COUNT, PIN_LED_DATA, NEO_GRBW + NEO_KHZ800);

enum InteractionState {
  IDLE,
  TOUCH_PENDING,    // Capacitive touch detected, waiting for thermal confirmation
  ACTIVE_MEASURED,  // Both sensors confirmed
  ACTIVE_ESTIMATED  // Thermal only, no touch
};

InteractionState state = IDLE;
unsigned long touchStartMs = 0;
unsigned long lastHeartbeatMs = 0;
float baselineAmbient = 0;
bool baselineSet = false;

// ── Evidence grade ───────────────────────────────────────────────
const char* gradeToString(InteractionState s) {
  switch (s) {
    case ACTIVE_MEASURED:  return "measured";
    case ACTIVE_ESTIMATED: return "estimated";
    default:               return "none";
  }
}

// ── LED colour mapping ──────────────────────────────────────────
// Maps temperature delta to a colour: cool blue → amber → warm gold
void updateLEDs(float delta) {
  uint8_t r, g, b, w;

  if (delta < 0.1f) {
    // Cool blue (idle)
    r = 0; g = 20; b = 80; w = 0;
  } else if (delta < TEMP_THRESHOLD) {
    // Transitioning: blue → amber
    float t = delta / TEMP_THRESHOLD;
    r = (uint8_t)(180 * t);
    g = (uint8_t)(80 * t);
    b = (uint8_t)(80 * (1.0f - t));
    w = (uint8_t)(20 * t);
  } else {
    // Warm gold (active contact)
    float intensity = min(delta / 3.0f, 1.0f);
    r = 200;
    g = (uint8_t)(120 * intensity);
    b = 0;
    w = (uint8_t)(80 * intensity);
  }

  for (int i = 0; i < LED_COUNT; i++) {
    leds.setPixelColor(i, leds.Color(r, g, b, w));
  }
  leds.show();
}

// ── Serial event output (JSON) ──────────────────────────────────
void publishEvent(const char* eventType, const char* grade,
                  float delta, float ambient, float object, int touchRaw) {
  Serial.print("{\"ts\":\"");
  Serial.print(millis()); // Replace with NTP timestamp when WiFi is configured
  Serial.print("\",\"type\":\"");
  Serial.print(eventType);
  Serial.print("\",\"grade\":\"");
  Serial.print(grade);
  Serial.print("\",\"delta_c\":");
  Serial.print(delta, 2);
  Serial.print(",\"ambient_c\":");
  Serial.print(ambient, 2);
  Serial.print(",\"object_c\":");
  Serial.print(object, 2);
  Serial.print(",\"touch_raw\":");
  Serial.print(touchRaw);
  Serial.println("}");
}

// ── Setup ────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Thermogenesis Prototype v0.1.0 ---");

  // I2C — set pins before mlx.begin() calls Wire.begin()
  Wire.setPins(PIN_SDA, PIN_SCL);

  if (!mlx.begin()) {
    Serial.println("ERROR: MLX90614 not found. Check wiring.");
    while (1) delay(1000);
  }
  Serial.println("MLX90614 connected.");

  leds.begin();
  leds.setBrightness(60);
  updateLEDs(0);
  Serial.println("LEDs initialised.");

  Serial.println("Ready. Reading at 1Hz.\n");
}

// ── Main loop (1Hz) ──────────────────────────────────────────────
void loop() {
  float ambient = mlx.readAmbientTempC();
  float object  = mlx.readObjectTempC();
  int   touchRaw = touchRead(PIN_TOUCH);

  // Establish ambient baseline on first valid reading
  if (!baselineSet && ambient > -40 && ambient < 80) {
    baselineAmbient = ambient;
    baselineSet = true;
    Serial.print("Baseline ambient: ");
    Serial.print(baselineAmbient, 1);
    Serial.println("°C");
  }

  float delta = object - ambient;
  bool thermalActive = (delta >= TEMP_THRESHOLD);
  bool thermalReset  = (delta < (TEMP_THRESHOLD - TEMP_HYSTERESIS));
  bool touched = (touchRaw > TOUCH_THRESHOLD);

  // ── State machine ──────────────────────────────────────────
  InteractionState prevState = state;

  switch (state) {
    case IDLE:
      if (touched) {
        state = TOUCH_PENDING;
        touchStartMs = millis();
        publishEvent("touch_begin", "pending", delta, ambient, object, touchRaw);
      } else if (thermalActive) {
        state = ACTIVE_ESTIMATED;
        publishEvent("proximity_begin", "estimated", delta, ambient, object, touchRaw);
      }
      break;

    case TOUCH_PENDING:
      if (thermalActive) {
        state = ACTIVE_MEASURED;
        publishEvent("confirmed", "measured", delta, ambient, object, touchRaw);
        lastHeartbeatMs = millis();
      } else if (!touched) {
        // Touch released before thermal confirmation
        publishEvent("touch_end", "brief", delta, ambient, object, touchRaw);
        state = IDLE;
      } else if (millis() - touchStartMs > CONFIRM_WINDOW_MS) {
        // Touch held but thermal never reached threshold — still classify as measured
        // (the person is touching but glass hasn't warmed enough)
        state = ACTIVE_MEASURED;
        publishEvent("confirmed_touch_only", "measured", delta, ambient, object, touchRaw);
        lastHeartbeatMs = millis();
      }
      break;

    case ACTIVE_MEASURED:
      if (!touched && thermalReset) {
        publishEvent("touch_end", "measured", delta, ambient, object, touchRaw);
        state = IDLE;
      } else if (millis() - lastHeartbeatMs > HEARTBEAT_MS) {
        publishEvent("heartbeat", "measured", delta, ambient, object, touchRaw);
        lastHeartbeatMs = millis();
      }
      break;

    case ACTIVE_ESTIMATED:
      if (touched) {
        state = ACTIVE_MEASURED;
        publishEvent("touch_begin", "measured", delta, ambient, object, touchRaw);
        lastHeartbeatMs = millis();
      } else if (thermalReset) {
        publishEvent("proximity_end", "estimated", delta, ambient, object, touchRaw);
        state = IDLE;
      }
      break;
  }

  // ── Update LEDs based on thermal delta ─────────────────────
  updateLEDs(delta);

  // ── Serial monitor output (human-readable) ─────────────────
  Serial.print("A:");
  Serial.print(ambient, 1);
  Serial.print("°C  O:");
  Serial.print(object, 1);
  Serial.print("°C  Δ:");
  Serial.print(delta, 2);
  Serial.print("°C  T:");
  Serial.print(touchRaw);
  Serial.print("  [");
  Serial.print(touched ? "TOUCH" : "-----");
  Serial.print("] ");
  Serial.print(thermalActive ? "THERMAL" : "-------");
  Serial.print("  state=");
  switch (state) {
    case IDLE:             Serial.print("IDLE");     break;
    case TOUCH_PENDING:    Serial.print("PENDING");  break;
    case ACTIVE_MEASURED:  Serial.print("MEASURED"); break;
    case ACTIVE_ESTIMATED: Serial.print("ESTIMATED"); break;
  }
  Serial.println();

  delay(1000); // 1Hz sampling
}
