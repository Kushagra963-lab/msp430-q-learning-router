import importlib.util
import sys
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).resolve().parents[1] / "simulator" / "simulate_network.py"
SPEC = importlib.util.spec_from_file_location("simulate_network", MODULE_PATH)
simulate_network = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = simulate_network
SPEC.loader.exec_module(simulate_network)


class SimulatorTest(unittest.TestCase):
    def test_state_encoder_uses_battery_link_and_queue(self):
        self.assertEqual(simulate_network.encode_state(90, 80, 10), 0)
        self.assertEqual(simulate_network.encode_state(90, 40, 10), 1)
        self.assertEqual(simulate_network.encode_state(20, 80, 10), 2)
        self.assertEqual(simulate_network.encode_state(20, 80, 90), 3)

    def test_simulation_is_repeatable_and_learns_routes(self):
        first = simulate_network.run(120, 11)
        second = simulate_network.run(120, 11)

        self.assertEqual(first, second)
        self.assertGreaterEqual(first["packet_delivery_ratio"], 0.0)
        self.assertLessEqual(first["packet_delivery_ratio"], 1.0)
        self.assertGreater(first["remaining_battery_percent"], 0.0)
        self.assertEqual(set(first["best_routes"]), {"S0", "S1", "S2", "S3"})


if __name__ == "__main__":
    unittest.main()
