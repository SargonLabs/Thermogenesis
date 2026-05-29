# evidence_grade — Confidence Metadata for Civic Sensor Observations

**Version:** 0.1.0 (draft)
**Licence:** CC-BY-SA-4.0
**Author:** William Janczewski / Sargon Labs

## Problem

Civic sensing platforms — Sensor.Community, Smart Citizen Kit, OpenEnergyMonitor — treat all sensor readings as equivalent. A calibrated reference instrument and a $5 consumer sensor produce observations that look identical in the database. There is no standard way to express how confident a platform is in a given reading, or what evidence supports it.

The OGC SensorThings API, the international standard for IoT sensor observations (OGC 15-078r6), defines a `resultQuality` field in its Observation entity. This field is intended to carry data quality metadata. In practice, it is almost universally empty — no standard vocabulary exists for what to put in it.

This document proposes `evidence_grade`: a typed, sensor-fusion-based confidence vocabulary for civic sensor observations.

## Specification

Each observation carries an `evidence_grade` field with one of three values:

| Grade | Definition | Evidence required |
|-------|-----------|-------------------|
| `measured` | High-confidence observation confirmed by multiple sensor inputs | Primary sensor reading above threshold AND secondary sensor confirmation within a defined time window |
| `estimated` | Medium-confidence observation from a single sensor input | Primary sensor reading above threshold, without secondary confirmation |
| `symbolic` | Explicit human or API contribution, not derived from sensor data | Deliberate entry via API or user interface |

## Design Principles

**Typed, not numeric.** Quality scores (0.0–1.0) are hard to interpret across different sensor types and deployments. A categorical grade with clear semantic meaning is more useful for downstream consumers and more honest about what the sensor system actually knows.

**Sensor-fusion-based.** The grade is determined by how many independent sensor inputs agree, not by a statistical model of sensor accuracy. This makes it applicable to any multi-sensor system, not just thermopile+capacitive sensing.

**Deployment-configurable.** The thresholds that determine grade boundaries are set per deployment, not hardcoded in the specification. A node in a warm environment has different baseline characteristics than one in a cool environment. The specification defines the grades and the decision logic; the deployment operator configures the thresholds.

**Separable from Thermogenesis.** The evidence_grade vocabulary is designed to be adopted by any civic sensing platform, regardless of what sensor type it uses. An air quality network could classify readings as `measured` (calibrated instrument with cross-reference), `estimated` (consumer sensor without calibration), or `symbolic` (user-reported observation). The vocabulary is not specific to thermal sensing.

## Reference Implementation: Thermogenesis

In Thermogenesis, evidence_grade is determined by fusing two independent sensor inputs:

**Primary sensor:** MLX90614 infrared thermopile, measuring inner glass surface temperature. The primary signal is `delta_c` — the difference between the object temperature and the ambient temperature.

**Secondary sensor:** ESP32-S3 native capacitive touch, detecting physical contact through the glass wall.

### Decision tree

```
                     ┌──────────────────────┐
                     │  delta_c >= threshold? │
                     └──────────┬───────────┘
                           ┌────┴────┐
                           │ NO      │ YES
                           ▼         ▼
                        ┌─────┐   ┌──────────────────────┐
                        │IDLE │   │ touch confirmed       │
                        └─────┘   │ within confirm_window?│
                                  └──────────┬───────────┘
                                        ┌────┴────┐
                                        │ NO      │ YES
                                        ▼         ▼
                                  ┌───────────┐ ┌──────────┐
                                  │ ESTIMATED │ │ MEASURED  │
                                  └───────────┘ └──────────┘
```

### Configurable parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `threshold` | 0.5 °C | Minimum delta_c to trigger grade assignment |
| `confirm_window_ms` | 10000 | Maximum time between capacitive touch and thermopile confirmation |
| `hysteresis_c` | 0.2 °C | Delta must drop below `threshold - hysteresis` before a new event triggers |

### Empirical results (breadboard prototype, May 2026)

| Test condition | delta_c | Response time | Touch signal | Grade |
|---------------|---------|---------------|-------------|-------|
| Direct IR, no glass, palm at 5cm | 6.8 °C | < 1 second | n/a | estimated |
| Through soda-lime glass (light fixture dome), sustained contact | 0.96 °C peak | ~10 seconds to threshold | 30,000 (baseline 23,000) | measured |
| Through soda-lime glass, hand removed | Decay from 0.7 to below 0.5 °C | ~12-15 seconds | Returns to baseline immediately | estimated → idle |

Glass composition: soda-lime (standard light fixture). Wall thickness: approximately 2-3mm. Ambient temperature: 24.5°C.

## Payload format

```json
{
  "ts": "2026-05-28T20:15:01.000Z",
  "grade": "measured",
  "delta_c": 0.96,
  "ambient_c": 24.5,
  "object_c": 25.4,
  "touch_raw": 30376,
  "confirm_delay_ms": 8200,
  "firmware_version": "0.1.0"
}
```

## Mapping to OGC SensorThings API

The evidence_grade field maps directly to the `resultQuality` property of the `Observation` entity in OGC SensorThings API (OGC 15-078r6, Section 8.2.3). The SensorThings specification defines `resultQuality` as type `DQ_Element` (from ISO 19157), which is broadly structured but rarely populated in practice.

A minimal SensorThings-compatible encoding:

```json
{
  "resultQuality": {
    "nameOfMeasure": "evidence_grade",
    "value": "measured",
    "description": "Thermopile delta >= threshold AND capacitive touch confirmed within confirmation window"
  }
}
```

This allows any SensorThings-compatible server to store and query evidence_grade metadata alongside standard observation data, without requiring changes to the SensorThings data model.

## Adoption path

The evidence_grade vocabulary is released under CC-BY-SA-4.0. Any civic sensing platform can adopt it by:

1. Adding an `evidence_grade` field to their observation payload
2. Defining which of their sensor inputs constitute primary evidence (for the measured/estimated distinction)
3. Configuring thresholds per deployment
4. Optionally mapping to the OGC SensorThings `resultQuality` field for standards compatibility

The vocabulary is intentionally minimal — three grades, clear semantics, no required infrastructure beyond a string field in the observation payload.

## References

- OGC SensorThings API Part 1: Sensing, Version 1.1 (OGC 18-088)
- OGC Observations and Measurements (ISO 19156:2011)
- ISO 19157: Geographic information — Data quality
- Sensor.Community (sensor.community)
- Smart Citizen Kit (smartcitizen.me)
- Thermogenesis project repository (github.com/SargonLabs/Thermogenesis)
