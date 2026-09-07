# SPDX-License-Identifier: GPL-3.0-only

import json
import os
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BINARY = ROOT / "build" / "sfree"

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


class SfreeCliTest(unittest.TestCase):
    maxDiff = None

    @classmethod
    def setUpClass(cls):
        subprocess.run(["cmake", "-S", str(ROOT), "-B", str(ROOT / "build")], check=True, cwd=ROOT)
        subprocess.run(["cmake", "--build", str(ROOT / "build")], check=True, cwd=ROOT)

    def run_sfree(self, *args, check=True, columns=None):
        with tempfile.NamedTemporaryFile("w", delete=False) as handle:
            handle.write(FAKE_MEMINFO)
            meminfo_path = handle.name

        try:
            env = os.environ.copy()
            env["SUPERFREE_MEMINFO"] = meminfo_path
            env["NO_COLOR"] = "1"
            if columns is not None:
                env["COLUMNS"] = str(columns)
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
            self.fail(f"sfree exited {result.returncode}\nSTDOUT:\n{result.stdout}\nSTDERR:\n{result.stderr}")
        return result

    def test_binary_and_help_use_sfree_command_name(self):
        self.assertTrue(BINARY.exists())

        help_result = self.run_sfree("--help")
        version_result = self.run_sfree("--version")
        invalid_result = self.run_sfree("--definitely-not-real", check=False)

        self.assertIn("Usage: sfree [OPTIONS]", help_result.stdout)
        self.assertIn("  sfree --json --unit MiB", help_result.stdout)
        self.assertEqual(version_result.stdout.strip(), "sfree 1.0.1")
        self.assertIn("sfree: unknown option: --definitely-not-real", invalid_result.stderr)
        self.assertIn("Try 'sfree --help'", invalid_result.stderr)

    def test_install_creates_usr_bin_symlink_when_prefix_is_usr_local(self):
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            build_dir = temp_root / "build"
            prefix = temp_root / "usr" / "local"
            usr_bin = temp_root / "usr" / "bin"

            subprocess.run([
                "cmake",
                "-S",
                str(ROOT),
                "-B",
                str(build_dir),
                f"-DCMAKE_INSTALL_PREFIX={prefix}",
                f"-DSFREE_USR_BIN_DIR={usr_bin}",
            ], check=True, cwd=ROOT)
            subprocess.run(["cmake", "--build", str(build_dir)], check=True, cwd=ROOT)
            subprocess.run(["cmake", "--install", str(build_dir)], check=True, cwd=ROOT)

            installed_binary = prefix / "bin" / "sfree"
            symlink = usr_bin / "sfree"
            manpage = prefix / "share" / "man" / "man1" / "sfree.1"

            self.assertTrue(installed_binary.exists())
            self.assertTrue(symlink.is_symlink())
            self.assertEqual(os.readlink(symlink), str(installed_binary))
            self.assertEqual(shutil.which("sfree", path=str(usr_bin)), str(symlink))
            self.assertTrue(manpage.exists())
            self.assertIn("sfree", manpage.read_text())

    def visible_width(self, line):
        plain = re.sub(r"\x1b\[[0-9;]*m", "", line)
        return len(plain)

    def assert_table_fits_and_is_aligned(self, output, columns):
        table_lines = [line.rstrip() for line in output.splitlines() if line.strip()]
        widths = [self.visible_width(line) for line in table_lines]
        self.assertTrue(widths, "expected table output")
        self.assertLessEqual(max(widths), columns)
        self.assertEqual(len(set(widths)), 1, widths)

    def test_json_output_is_machine_readable_and_uses_selected_unit(self):
        result = self.run_sfree("--json", "--unit", "MiB")

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
        result = self.run_sfree("--bytes", "--total", "--wide")

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
        result = self.run_sfree("--line", "--bytes", "--total", "--committed")

        output = result.stdout.strip()

        self.assertIn("MemTotal 4194304000", output)
        self.assertIn("MemUsed 2097152000", output)
        self.assertIn("SwapTotal 1048576000", output)
        self.assertIn("TotalUsed 2883584000", output)
        self.assertIn("CommitLimit 8388608000", output)
        self.assertIn("Committed_AS 2097152000", output)

    def test_free_short_h_means_human_readable_not_help(self):
        result = self.run_sfree("-h", "--total")

        self.assertIn("Mem:", result.stdout)
        self.assertIn("Swap:", result.stdout)
        self.assertIn("Total:", result.stdout)
        self.assertNotIn("Usage:", result.stdout)

    def test_invalid_option_exits_nonzero_with_clear_error(self):
        result = self.run_sfree("--definitely-not-real", check=False)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unknown option: --definitely-not-real", result.stderr)
        self.assertIn("Try 'sfree --help'", result.stderr)

    def test_default_table_adapts_to_terminal_width(self):
        result = self.run_sfree("--color", "never", columns=60)

        self.assert_table_fits_and_is_aligned(result.stdout, 60)
        self.assertIn("TYPE", result.stdout)
        self.assertIn("USE%", result.stdout)
        self.assertNotIn("BUF/CACHE", result.stdout)
        self.assertNotIn("AVAILABLE", result.stdout)

    def test_default_table_keeps_medium_layout_within_terminal_width(self):
        result = self.run_sfree("--color", "never", columns=100)

        self.assert_table_fits_and_is_aligned(result.stdout, 100)
        self.assertIn("TYPE", result.stdout)
        self.assertIn("TOTAL", result.stdout)
        self.assertIn("USED", result.stdout)
        self.assertIn("FREE", result.stdout)
        self.assertIn("USE%", result.stdout)

    def test_default_table_has_ultra_compact_layout_for_tiny_terminals(self):
        result = self.run_sfree("--color", "never", columns=40)

        self.assert_table_fits_and_is_aligned(result.stdout, 40)
        self.assertIn("TYPE", result.stdout)
        self.assertIn("USED", result.stdout)
        self.assertIn("USE%", result.stdout)
        self.assertNotIn("TOTAL", result.stdout)
        self.assertNotIn("FREE", result.stdout)


if __name__ == "__main__":
    unittest.main()
