# Breadboard Prototype Test Results

**Date:** May 28, 2026
**Location:** Sargon Labs, Bethlehem, PA
**Hardware:** ESP32-S3-WROOM-1 (N16R8), MLX90614ESF (GY-906 breakout), SK6812 RGBW LEDs, capacitive touch via copper foil on glass
**Glass:** Soda-lime globe (light fixture dome), estimated wall thickness 2-3mm
**Ambient temperature:** 24.4–24.8°C
**Firmware:** Thermogenesis Phase 2 (see firmware/src/main.cpp)

## Test A — Direct IR (no glass)

Palm flat, 5cm above sensor.

| Metric | Value |
|--------|-------|
| Ambient | 24.5°C |
| Object (peak) | 31.3°C |
| Delta (peak) | 6.8°C |
| Response time | < 1 second |
| State | ESTIMATED (no touch sensor in path) |

## Test B — Through soda-lime glass, sustained contact

Palm pressed flat against outside of glass dome, copper tape electrode on inside surface, MLX90614 positioned inside dome pointing at glass wall.

| Metric | Value |
|--------|-------|
| Ambient | 24.5–24.8°C |
| Object (peak) | 25.5°C |
| Delta (peak) | 0.96°C |
| Time to 0.5°C threshold | ~8–10 seconds |
| Time to peak | ~120 seconds (2 minutes) |
| Capacitive touch baseline | ~23,000 |
| Capacitive touch (through glass) | ~29,000–30,400 |
| Touch threshold used | 27,000 |
| State | MEASURED (touch + thermal confirmed) |

## Test C — Thermal decay after hand removal

Hand removed after sustained contact.

| Time after removal | Delta | State |
|-------------------|-------|-------|
| 0 seconds | 0.76°C | ESTIMATED |
| 3 seconds | 0.68°C | ESTIMATED |
| 6 seconds | 0.60°C | ESTIMATED |
| 9 seconds | 0.54°C | ESTIMATED |
| 12 seconds | 0.46°C | IDLE |
| 15 seconds | 0.38°C | IDLE |

Decay from above-threshold to below-threshold: approximately 12-15 seconds.

## Observations

1. **Conduction-based sensing confirmed.** The glass is opaque to the MLX90614's 5.5–14μm sensing band. The sensor reads inner glass surface temperature, which rises through thermal conduction from body contact on the outside. This means any glass composition works.

2. **Capacitive touch through glass confirmed.** Signal attenuated from 350,000+ (direct contact with bare wire) to ~30,000 (through 2-3mm soda-lime glass). Still well above a detectable threshold.

3. **Two-signal timing asymmetry confirmed.** Capacitive touch triggers within milliseconds. Thermal delta takes 8–10 seconds to cross the 0.5°C threshold. The evidence_grade decision tree correctly handles this: touch fires first (PENDING), thermal confirms later (MEASURED).

4. **Thermal persistence after release.** The glass retains heat for 12–15 seconds after hand removal. During this period, the system correctly transitions from MEASURED to ESTIMATED (thermal without touch) to IDLE.

5. **Touch signal instability near threshold.** Capacitive values through glass fluctuate between 26,000 and 30,400 during sustained contact, occasionally dropping below the 27,000 threshold. This causes brief MEASURED → ESTIMATED → MEASURED transitions during a single sustained touch. Production firmware should add hysteresis or a debounce timer to the touch classification.
