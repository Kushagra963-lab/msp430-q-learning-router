# Architecture

The project separates the learning engine from hardware-specific I/O so the same Q-learning logic can run in a host simulator and on MSP430 firmware.

## Data Flow

```text
ADC battery reading
RSSI / link-quality estimate
TX queue depth
Packet delivery feedback
Latency measurement
        |
        v
ql_encode_state()
        |
        v
ql_select_action()
        |
        v
radio_send_packet()
        |
        v
ql_calculate_reward()
        |
        v
ql_update()
```

## Memory Profile

The default firmware model uses four states and three actions.

```text
4 states * 3 actions * 4-byte float = 48 bytes
```

The implementation keeps the public API small and avoids heap allocation.

## Learning Parameters

The firmware defaults match the project brief:

```text
alpha = 0.2
gamma = 0.8
epsilon = 0.1
```

The host simulator passes the exploration roll into `ql_select_action()` so embedded builds can provide randomness from a timer, LFSR, ADC noise, or a radio-derived entropy source.

## Hardware Boundary

The learning engine does not assume a specific radio module. A board integration should implement:

- `radio_send_packet()`
- `radio_receive_packet()`
- `read_battery_mv()`
- `read_link_quality()`
- `read_queue_load()`
- `now_ms()`

This keeps nRF24L01+, CC1101, and SX1278 integrations isolated from the reinforcement-learning core.

