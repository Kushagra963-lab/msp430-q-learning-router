# MSP430 Q-Learning Router

Energy-aware distributed routing for wireless sensor networks on MSP430-class microcontrollers.

This project implements the core of a Q-learning based next-hop selector for resource-constrained sensor nodes. Each node observes battery level, link quality, queue load, delivery success, energy cost, and latency, then updates a compact Q-table to prefer routes that preserve lifetime while keeping delivery reliable.

Live dashboard: https://kushagra963-lab.github.io/msp430-q-learning-router/

Deployment: every push to `main` that changes `web/` or the deploy workflow republishes the dashboard to the `gh-pages` branch with GitHub Actions.

## What Is Included

- Embedded C routing engine for MSP430 targets.
- Portable host simulation build for local testing without hardware.
- Python network simulator for repeatable routing experiments.
- Static web dashboard that visualizes topology, Q-values, convergence, packet delivery, energy, and latency.
- Deployment notes for MSP430 LaunchPad nodes and radio modules.

## Architecture

```text
Sensor inputs
    |
State builder
    |
Q-learning engine
    |
Route selector
    |
Radio driver hook
    |
Wireless sensor network
```

## State And Action Model

The default state space is intentionally small for MSP430 RAM:

| State | Battery | Link quality |
| --- | --- | --- |
| S0 | High | Good |
| S1 | High | Poor |
| S2 | Low | Good |
| S3 | Low | Poor |

Actions represent next-hop candidates. The default firmware profile uses three actions:

```text
Action 0 = neighbor A
Action 1 = neighbor B
Action 2 = gateway or relay
```

The default Q-table is `4 x 3 x sizeof(float) = 48 bytes`.

## Reward Function

```text
reward = delivery_bonus - energy_cost - latency_penalty
```

Delivered packets receive a positive reward. Drops, high energy use, and high latency reduce the score.

## Quick Start

Run the portable C simulation:

```bash
cd firmware
make host
./build/ql-router-sim
```

Run the Python network simulator:

```bash
python3 simulator/simulate_network.py --episodes 240 --seed 7
```

Open the dashboard locally:

```bash
python3 -m http.server 4173 --directory web
```

Then visit `http://localhost:4173`.

## MSP430 Firmware Build

Install an MSP430 GCC toolchain and build:

```bash
cd firmware
make firmware
```

Flash with MSPDebug:

```bash
make flash
```

The firmware exposes hardware hooks in `firmware/src/main.c` for battery ADC, radio send, radio receive, and node ID selection.

## Radio Modules

The routing engine is radio-agnostic. It can sit above drivers for:

- nRF24L01+
- CC1101
- LoRa SX1278

## Metrics

The simulators report:

- Network lifetime proxy
- Average remaining energy
- Packet delivery ratio
- Average latency
- Learned best route per state

## Repository Layout

```text
firmware/       Embedded C core, host build, MSP430 build targets
simulator/      Deterministic Python network simulation
web/            Deployable static dashboard
docs/           Architecture and deployment notes
```
