# Thermogenesis

Open-source hardware and software platform for non-contact thermal sensing of human presence in shared spaces.

A person touches or stands near a hand-blown glass enclosure containing an infrared thermopile sensor. The system measures body heat conducted through the glass, classifies the interaction by confidence level, and publishes it to a shared open dataset. Deployed across a network of institutions — schools, libraries, community centres, museums — the platform builds a civic record of human thermal presence without cameras, without identifiable data, and without proprietary infrastructure.

**Status:** Active R&D at [Sargon Labs](https://sargonlabs.com), Bethlehem, Pennsylvania.

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

- **measured** — thermopile delta ≥ 0.5°C AND capacitive touch confirmed
- **estimated** — thermopile delta detected without touch confirmation
- **symbolic** — explicitly contributed via the REST API

## Tech stack

| Layer | Technology | Licence |
|-------|-----------|---------|
| PCB schematics & layout | KiCad | CERN-OHL-S-2.0 |
| Enclosure design | FreeCAD | CERN-OHL-S-2.0 |
| Firmware | ESP32 C++ / PlatformIO | GPL-3.0 |
| Backend | PostgreSQL, TimescaleDB, Mosquitto | AGPL-3.0 |
| Deployment | Docker Compose | AGPL-3.0 |
| Protocol & documentation | MQTT + JSON schema | CC-BY-SA-4.0 |

## Repository structure

```
thermogenesis/
├── firmware/          # ESP32-S3 firmware (PlatformIO)
├── hardware/          # PCB schematics, Gerber files (KiCad)
├── enclosure/         # Glass mold profiles, mounting specs (FreeCAD)
├── backend/           # MQTT broker config, DB schema, API, visualisation
├── docs/              # Protocol spec, deployment guide, calibration
│   └── protocol.md    # MQTT topic structure and payload format
├── LICENSE            # GPL-3.0 (firmware, default)
├── hardware/LICENSE   # CERN-OHL-S-2.0
└── CONTRIBUTING.md
```

## Hardware

- **MCU:** ESP32-S3-WROOM-1 (dual-core 240MHz, Wi-Fi, BLE, native USB, capacitive touch)
- **IR sensor:** MLX90614ESF (5.5–14μm far-IR thermopile, I2C, ±0.5°C accuracy)
- **LEDs:** SK6812 RGBW (addressable, warm white channel)
- **Touch:** Capacitive sensing via ESP32-S3 native touch pins, coupled through glass wall
- **Power:** USB-C PD
- **PCB:** Stacked design, ~35mm diameter

## How it works

The MLX90614 thermopile reads the temperature of the inner glass surface. When a person touches or stands near the outside of the glass, body heat conducts through the wall and raises the inner surface temperature. The sensor detects this change. Capacitive touch sensing through the glass provides an immediate contact signal. The firmware fuses both inputs to classify each interaction.

The glass enclosure is not an IR-transparent window — it acts as a thermal intermediary. This means any glass composition works (soda-lime, borosilicate, hand-blown art glass), since the sensor measures conduction-driven surface temperature change, not transmitted radiation.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## Contact

William Janczewski — Sargon Labs, Bethlehem, PA
