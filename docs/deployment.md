# Deployment Guide

## Minimum Hardware

- MSP430G2553, MSP430FR5969, or MSP430F5529.
- MSP430 LaunchPad or compatible programmer.
- nRF24L01+, CC1101, or LoRa SX1278 radio module.
- Coin-cell, Li-ion, or bench supply.
- Gateway node connected to a PC over UART.

## Firmware Steps

1. Set `NODE_ID` and neighbor IDs in the board profile.
2. Connect the radio module over SPI.
3. Calibrate the battery ADC divider.
4. Build the firmware:

```bash
cd firmware
make firmware
```

5. Flash each node:

```bash
make flash
```

6. Power the gateway first, then relay nodes, then source nodes.
7. Send several hundred packets to allow the Q-values to converge.

## Example Topology

```text
Node 1 ---- Node 2 ---- Gateway
   |           |
   +---- Node 3
```

## Measurements

Capture these metrics at the gateway:

- Packets sent and delivered.
- Per-hop latency.
- Retransmissions.
- Battery voltage over time.
- Learned best action for each state.

## Expected Behavior

Initial packets explore multiple routes. Over time, nodes should prefer the path with stronger link quality and lower energy cost. If a relay battery drops, the state encoder moves the node into a low-battery state, and the learned policy can shift traffic away from that relay.

