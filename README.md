# Thermogenesis

Open-source hardware and software platform for non-contact thermal sensing of human presence in shared spaces.

A person touches a glass enclosure containing an infrared thermopile sensor. Body heat conducts through the glass wall and the sensor reads the temperature change. Capacitive touch detects contact instantly. The firmware fuses both signals, classifies the interaction by confidence level, and publishes it to a shared dataset via MQTT. Deployed across a network of institutions, the platform builds a civic record of human thermal presence without cameras, without identifiable data, and without proprietary infrastructure.

**Status:** Active R&D at [Sargon Labs](https://sargonlabs.com), Bethlehem, Pennsylvania.

## Prototype Demo

Working breadboard prototype: ESP32-S3, MLX90614 thermopile, capacitive touch through soda-lime glass, SK6812 RGBW LED thermal response, evidence_grade state machine.

**Video:** [Breadboard prototype demo](https://youtu.be/ObmN-5IK4TE)

### Breadboard Test Results (May 2026)

| Test | Delta | Response | Grade |
|------|-------|----------|-------|
| Direct IR, no glass | 6.8°C | < 1 second | estimated |
| Through soda-lime glass, sustained contact | 0.96°C peak | ~10 seconds to threshold | measured |
| Thermal decay after hand removal | below 0.5°C | ~12-15 seconds | idle |

Glass is opaque at the thermopile's 5.5-14μm wavelength. The sensor reads conduction-driven surface temperature change, not transmitted IR. Any glass composition works.

Full test data: [docs/test-results_v01_prototype.md](docs/test-results_v01_prototype.md)

## Architecture

```
┌─────────────────────────────┐
│  Glass enclosure            │
│  ┌────────────────────────┐ │
│  │ ESP32-S3               │ │
│  │ MLX90614 thermopile    │ │
│  │ Capacitive touch (GPIO)│ │
│  │ SK6812 RGBW LEDs       │ │
│  │ USB-C PD power         │ │
│  └────────────────────────┘ │
└─────────────┬───────────────┘
              │ MQTT / TLS
              ▼
┌─────────────────────────────┐
│ MQTT broker (Mosquitto)     │
└─────────────┬───────────────┘
              │
              ▼
┌─────────────────────────────┐
│ PostgreSQL + TimescaleDB    │
│ Time-series thermal data    │
└─────────────┬───────────────┘
              │
              ▼
┌─────────────────────────────┐
│ REST API + web visualisation│
└─────────────────────────────┘
```

Each node operates independently with offline flash buffering. When connectivity is available, data flows to the shared backend. Every interaction is tagged with an `evidence_grade`:

- **measured** — capacitive touch detected AND thermopile delta ≥ 0.5°C confirmed within 10-second conduction window
- **estimated** — thermopile delta detected without touch confirmation
- **symbolic** — explicitly contributed via the REST API

The `evidence_grade` taxonomy is designed as a candidate vocabulary for the `resultQuality` field in the OGC SensorThings API. See the [evidence_grade specification](docs/evidence-grade-spec.md).

## Tech Stack

| Layer | Technology | Licence |
|-------|-----------|---------|
| PCB schematics & layout | KiCad | CERN-OHL-S-2.0 |
| Enclosure design | FreeCAD | CERN-OHL-S-2.0 |
| Firmware | ESP32 C++ / PlatformIO | GPL-3.0 |
| Backend | PostgreSQL, TimescaleDB, Mosquitto | AGPL-3.0 |
| Deployment | Docker Compose | AGPL-3.0 |
| Protocol & documentation | MQTT + JSON schema | CC-BY-SA-4.0 |

## Repository Structure

```
thermogenesis/
├── firmware/
│   ├── src/
│   │   └── main.cpp          # ESP32-S3 firmware (sensor fusion, LEDs, evidence_grade)
│   └── platformio.ini         # PlatformIO build configuration
├── hardware/
│   ├── LICENSE                # CERN-OHL-S-2.0
│   └── thermogenesis_schematic_v0.1.png  # Breadboard prototype schematic
├── docs/
│   ├── protocol.md            # MQTT topic structure and payload format
│   ├── evidence-grade-spec.md # evidence_grade specification with OGC SensorThings mapping
│   ├── test-results_v01_prototype.md  # Breadboard empirical data
│   └── Thermogenesis_Architecture.pdf # System architecture diagram
├── enclosure/                 # Glass mold profiles, mounting specs (FreeCAD) — future
├── backend/                   # MQTT broker config, DB schema, API — future
├── LICENSE                    # GPL-3.0 (firmware, default)
└── CONTRIBUTING.md
```

## Hardware

- **MCU:** ESP32-S3-WROOM-1 (dual-core 240MHz, Wi-Fi, BLE, native USB, capacitive touch)
- **IR sensor:** MLX90614ESF (5.5–14μm far-IR thermopile, I2C, ±0.5°C accuracy)
- **LEDs:** SK6812 RGBW (addressable, warm white channel)
- **Touch:** Capacitive sensing via ESP32-S3 native touch pins, coupled through glass wall with copper foil electrode
- **Power:** USB-C (3.3V from onboard LDO; 5V pin inactive on UART port)
- **PCB:** Stacked design, ~35mm diameter (target production form factor)

## How It Works

The MLX90614 thermopile reads the temperature of the inner glass surface. When a person touches the outside of the glass, body heat conducts through the wall and raises the inner surface temperature. The sensor detects this change. Capacitive touch sensing through the glass provides an immediate contact signal. The firmware fuses both inputs to classify each interaction.

The glass enclosure is not an IR-transparent window. Both borosilicate and soda-lime glass are opaque in the MLX90614's 5.5-14μm sensing band. The glass acts as a thermal intermediary: body heat conducts through the wall, the inner surface warms, and the thermopile reads the surface temperature change. This means the conduction approach works across glass compositions, though the signal quality varies with thickness and thermal conductivity.

Capacitive touch fires instantly. The thermal signal takes 3-15 seconds depending on glass thickness. The evidence_grade decision tree handles this asymmetry: touch triggers first, then the system waits up to 10 seconds for thermal confirmation.

## Documentation

- [MQTT Protocol Specification](docs/protocol.md)
- [evidence_grade Specification](docs/evidence-grade-spec.md) — confidence metadata for civic sensor observations, with OGC SensorThings mapping
- [Breadboard Test Results](docs/test-results_v01_prototype.md)
- [System Architecture (PDF)](docs/Thermogenesis_Architecture.pdf)
- [Breadboard Schematic](hardware/thermogenesis_schematic_v0.1.png)

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## Contact

William Janczewski — [Sargon Labs](https://sargonlabs.com), Bethlehem, PA
