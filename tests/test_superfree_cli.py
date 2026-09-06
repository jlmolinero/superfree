# SPDX-License-Identifier: GPL-3.0-only

import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BINARY = ROOT / "build" / "superfree"

FAKE_MEMINFO = """\
MemTotal:        4096000 kB
MemFree:         1024000 kB
MemAvailable:   2048000 kB
Buffers:          256000 kB
Cached:           512000 kB
Shmem:             64000 kB
SReclaimable:     128000 kB
SwapTotal:       1024000 kB
SwapFree:         256000 kB
HighTotal:       3072000 kB
HighFree:         768000 kB
LowTotal:        1024000 kB
LowFree:          256000 kB
CommitLimit:     8192000 kB
Committed_AS:    2048000 kB
"""


class SuperfreeCliTest(unittest.TestCase):
    maxDiff = None

    @classmethod
    def setUpClass(cls):
        subprocess.run(["cmake", "-S", str(ROOT), "-B", str(ROOT / "build")], check=True, cwd=ROOT)
        subprocess.run(["cmake", "--build", str(ROOT / "build")], check=True, cwd=ROOT)

    def run_superfree(self, *args, check=True):
        with tempfile.NamedTemporaryFile("w", delete=False) as handle:
            handle.write(FAKE_MEMINFO)
            meminfo_path = handle.name

        try:
            env = os.environ.copy()
            env["SUPERFREE_MEMINFO"] = meminfo_path
            env["NO_COLOR"] = "1"
            result = subprocess.run(
                [str(BINARY), *args],
                cwd=ROOT,
                env=env,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            )
        finally:
            os.unlink(meminfo_path)

        if check and result.returncode != 0:
            self.fail(f"superfree exited {result.returncode}\nSTDOUT:\n{result.stdout}\nSTDERR:\n{result.stderr}")
        return result

    def test_json_output_is_machine_readable_and_uses_selected_unit(self):
        result = self.run_superfree("--json", "--unit", "MiB")

        payload = json.loads(result.stdout)

        self.assertEqual(payload["unit"], "MiB")
        self.assertFalse(payload["si"])
        self.assertEqual(payload["memory"]["total"], 4000.0)
        self.assertEqual(payload["memory"]["used"], 2000.0)
        self.assertEqual(payload["memory"]["free"], 1000.0)
        self.assertEqual(payload["memory"]["shared"], 62.5)
        self.assertEqual(payload["memory"]["buff_cache"], 875.0)
        self.assertEqual(payload["memory"]["available"], 2000.0)
        self.assertEqual(payload["swap"], {"total": 1000.0, "used": 750.0, "free": 250.0, "usage_percent": 75.0})
        self.assertEqual(payload["total"], {"total": 5000.0, "used": 2750.0, "free": 1250.0, "usage_percent": 55.0})

    def test_free_compatible_bytes_total_wide_output(self):
        result = self.run_superfree("--bytes", "--total", "--wide")

        lines = [line.rstrip() for line in result.stdout.splitlines()]

        self.assertIn("total", lines[0])
        self.assertIn("buffers", lines[0])
        self.assertIn("cache", lines[0])
        self.assertIn("available", lines[0])
        self.assertIn("Mem:", lines[1])
        self.assertIn("4194304000", lines[1])
        self.assertIn("2097152000", lines[1])
        self.assertIn("1048576000", lines[1])
        self.assertIn("65536000", lines[1])
        self.assertIn("262144000", lines[1])
        self.assertIn("655360000", lines[1])
        self.assertIn("2097152000", lines[1])
        self.assertIn("Swap:", lines[2])
        self.assertIn("Total:", lines[3])
        self.assertIn("5242880000", lines[3])

    def test_free_compatible_line_output_includes_requested_rows(self):
        result = self.run_superfree("--line", "--bytes", "--total", "--committed")

        output = result.stdout.strip()

        self.assertIn("MemTotal 4194304000", output)
        self.assertIn("MemUsed 2097152000", output)
        self.assertIn("SwapTotal 1048576000", output)
        self.assertIn("TotalUsed 2883584000", output)
        self.assertIn("CommitLimit 8388608000", output)
        self.assertIn("Committed_AS 2097152000", output)

    def test_free_short_h_means_human_readable_not_help(self):
        result = self.run_superfree("-h", "--total")

        self.assertIn("Mem:", result.stdout)
        self.assertIn("Swap:", result.stdout)
        self.assertIn("Total:", result.stdout)
        self.assertNotIn("Usage:", result.stdout)

    def test_invalid_option_exits_nonzero_with_clear_error(self):
        result = self.run_superfree("--definitely-not-real", check=False)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unknown option: --definitely-not-real", result.stderr)
        self.assertIn("Try 'superfree --help'", result.stderr)


if __name__ == "__main__":
    unittest.main()
