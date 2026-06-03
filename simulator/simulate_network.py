#!/usr/bin/env python3
"""Deterministic Q-learning network simulation for the MSP430 router project."""

from __future__ import annotations

import argparse
import json
import random
from dataclasses import dataclass, field

ALPHA = 0.2
GAMMA = 0.8
EPSILON = 0.1
STATE_COUNT = 4
ACTION_COUNT = 3


@dataclass
class Link:
    name: str
    base_quality: float
    energy_cost: float
    latency_ms: float
    drop_penalty: float


@dataclass
class Router:
    q: list[list[float]] = field(default_factory=lambda: [[0.0 for _ in range(ACTION_COUNT)] for _ in range(STATE_COUNT)])

    def best_action(self, state: int) -> int:
        return max(range(ACTION_COUNT), key=lambda action: self.q[state][action])

    def select_action(self, state: int, rng: random.Random) -> int:
        if rng.random() < EPSILON:
            return rng.randrange(ACTION_COUNT)
        return self.best_action(state)

    def update(self, state: int, action: int, reward: float, next_state: int) -> None:
        current = self.q[state][action]
        target = reward + GAMMA * max(self.q[next_state])
        self.q[state][action] = current + ALPHA * (target - current)


def encode_state(battery_percent: float, link_quality: float, queue_load: float) -> int:
    low_battery = battery_percent < 35.0
    poor_link = link_quality < 55.0 or queue_load > 75.0
    if low_battery and poor_link:
        return 3
    if low_battery:
        return 2
    if poor_link:
        return 1
    return 0


def reward(delivered: bool, energy_cost: float, latency_ms: float) -> float:
    return (10.0 if delivered else -10.0) - energy_cost - latency_ms / 8.0


def run(episodes: int, seed: int) -> dict[str, object]:
    rng = random.Random(seed)
    router = Router()
    links = [
        Link("Node 2 relay", 78.0, 2.2, 16.0, 0.08),
        Link("Node 3 relay", 63.0, 2.8, 20.0, 0.16),
        Link("Gateway direct", 47.0, 4.6, 12.0, 0.28),
    ]
    battery = 100.0
    delivered_count = 0
    total_latency = 0.0
    total_energy = 0.0
    history = []

    for episode in range(episodes):
        queue_load = float((episode * 13) % 100)
        observed_quality = max(5.0, links[0].base_quality - queue_load * 0.08 + rng.uniform(-7.0, 7.0))
        state = encode_state(battery, observed_quality, queue_load)
        action = router.select_action(state, rng)
        link = links[action]
        link_quality = max(5.0, min(96.0, link.base_quality - queue_load * 0.06 + rng.uniform(-9.0, 9.0)))
        success_threshold = max(0.05, min(0.98, link_quality / 100.0 - link.drop_penalty))
        delivered = rng.random() < success_threshold
        latency = link.latency_ms + queue_load * 0.05 + rng.uniform(0.0, 3.5)
        energy = link.energy_cost + (0.8 if not delivered else 0.0) + queue_load * 0.01
        battery = max(0.0, battery - energy * 0.045)
        next_state = encode_state(battery, link_quality, queue_load)
        episode_reward = reward(delivered, energy, latency)

        router.update(state, action, episode_reward, next_state)

        delivered_count += 1 if delivered else 0
        total_latency += latency
        total_energy += energy

        if episode % max(1, episodes // 12) == 0 or episode == episodes - 1:
            history.append(
                {
                    "episode": episode,
                    "battery": round(battery, 2),
                    "pdr": round(delivered_count / (episode + 1), 3),
                    "best_action_s0": links[router.best_action(0)].name,
                }
            )

    return {
        "episodes": episodes,
        "seed": seed,
        "packet_delivery_ratio": round(delivered_count / episodes, 3),
        "average_latency_ms": round(total_latency / episodes, 2),
        "average_energy_cost": round(total_energy / episodes, 2),
        "remaining_battery_percent": round(battery, 2),
        "best_routes": {f"S{state}": links[router.best_action(state)].name for state in range(STATE_COUNT)},
        "q_table": [[round(value, 3) for value in row] for row in router.q],
        "history": history,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Simulate Q-learning based WSN routing.")
    parser.add_argument("--episodes", type=int, default=240)
    parser.add_argument("--seed", type=int, default=7)
    args = parser.parse_args()

    if args.episodes <= 0:
        raise SystemExit("--episodes must be positive")

    print(json.dumps(run(args.episodes, args.seed), indent=2))


if __name__ == "__main__":
    main()

