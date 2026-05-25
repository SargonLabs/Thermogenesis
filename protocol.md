# Thermogenesis MQTT Protocol Specification

**Version:** 0.1.0 (draft)
**Licence:** CC-BY-SA-4.0

## Topic structure

```
thermogenesis/{site_id}/{node_id}/{message_type}
```

- `site_id` — unique identifier for the deployment site (e.g. `broughal-ms`)
- `node_id` — unique identifier for the node at that site (e.g. `node-001`)
- `message_type` — one of: `event`, `heartbeat`, `status`

### Examples

```
thermogenesis/broughal-ms/node-001/event
thermogenesis/broughal-ms/node-001/heartbeat
thermogenesis/broughal-ms/node-001/status
```

## Message types

### event

Published on state change: touch begin, touch end, or evidence_grade transition.

```json
{
  "ts": "2026-05-27T14:32:01.000Z",
  "type": "touch_begin",
  "grade": "measured",
  "delta_c": 1.4,
  "ambient_c": 22.1,
  "object_c": 23.5,
  "touch_raw": 72000,
  "firmware_version": "0.1.0"
}
```

| Field | Type | Description |
|-------|------|-------------|
| `ts` | ISO 8601 string | Timestamp (UTC) |
| `type` | string | `touch_begin`, `touch_end`, `proximity_begin`, `proximity_end` |
| `grade` | string | `measured`, `estimated`, `symbolic` |
| `delta_c` | float | Object temperature minus ambient temperature (°C) |
| `ambient_c` | float | MLX90614 ambient (die) temperature (°C) |
| `object_c` | float | MLX90614 object (glass surface) temperature (°C) |
| `touch_raw` | integer | Raw capacitive touch sensor value |
| `firmware_version` | string | Semver firmware version |

### heartbeat

Published every 60 seconds during sustained contact. Same schema as `event`, with `type: "heartbeat"`.

### status

Published on boot, on network reconnect, and every 5 minutes when idle.

```json
{
  "ts": "2026-05-27T14:30:00.000Z",
  "type": "boot",
  "firmware_version": "0.1.0",
  "uptime_s": 0,
  "wifi_rssi": -42,
  "flash_buffered_events": 0,
  "free_heap_bytes": 245760
}
```

## evidence_grade classification

| Grade | Condition | Meaning |
|-------|-----------|---------|
| `measured` | `delta_c >= threshold` AND capacitive touch confirmed within confirmation window | Direct human contact detected by both sensors |
| `estimated` | `delta_c >= threshold` without capacitive touch | Thermal presence detected nearby (body heat without touch) |
| `symbolic` | Submitted via REST API | Deliberate non-sensor contribution |

### Parameters (configurable per deployment)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `threshold` | 0.5 °C | Minimum temperature delta to trigger classification |
| `confirmation_window_ms` | 10000 | Max time between capacitive touch and thermopile confirmation |
| `hysteresis_c` | 0.2 °C | Delta must drop below `threshold - hysteresis` before a new event triggers |
| `heartbeat_interval_ms` | 60000 | Interval between heartbeat messages during sustained contact |

## Transport

- **Protocol:** MQTT v3.1.1 over TLS (port 8883)
- **QoS:** 1 (at least once delivery)
- **Offline buffering:** Events buffered to ESP32 flash ring buffer during connectivity loss, published in order on reconnect
- **Broker:** Mosquitto (reference deployment)

## Data types

All timestamps are ISO 8601 UTC. All temperatures are Celsius. All payloads are JSON (UTF-8).
