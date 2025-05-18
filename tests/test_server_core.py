#!/usr/bin/env python3

import unittest
import subprocess
import signal
import time
import os

TMP_DIR = "tests/tmp"
CONFIG_PATH = os.path.join(TMP_DIR, "test.conf")

def run_and_capture(command, timeout=2):
    proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        out, err = proc.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        proc.kill()
        raise AssertionError("Subprocess timed out")
    return proc.returncode, out, err

class ServerCoreTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        os.makedirs(TMP_DIR, exist_ok=True)

    def test_00_missing_argument(self):
        code, out, err = run_and_capture(["./webserv"])
        self.assertEqual(code, 1)
        self.assertIn(b"Error: Usage: ./webserv <config_file>.conf", err)
        self.assertEqual(out, b"", "Expected no output to stdout")

    def test_01_empty_config_file(self):
        with open(CONFIG_PATH, "w") as f:
            f.write("")

        code, out, err = run_and_capture(["./webserv", CONFIG_PATH])
        self.assertEqual(code, 1)
        self.assertIn(b"Error: empty config", err)
        self.assertEqual(out, b"", "Expected no output to stdout")

#     def test_02_server_starts(self):
#         with open(CONFIG_PATH, "w") as f:
#             f.write("""
# server {
#     listen 8080;
#     root tests/www;
# }
# """)
#         proc = subprocess.Popen(["./webserv", CONFIG_PATH])
#         time.sleep(0.5)
#         self.assertIsNone(proc.poll(), "Server process is not running")
#         proc.send_signal(signal.SIGINT)
#         _, _ = proc.communicate(timeout=2)

if __name__ == "__main__":
    unittest.main()
