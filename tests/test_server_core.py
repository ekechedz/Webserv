#!/usr/bin/env python3

import unittest
import subprocess
import signal
import time
import os
import textwrap

# Temporary directory and path for test config files
TMP_DIR = "tests/tmp"
CONFIG_PATH = os.path.join(TMP_DIR, "test.conf")

# Helper function to run a command and capture its output
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
        # Make sure the tmp test directory exists
        os.makedirs(TMP_DIR, exist_ok=True)

    def test_00_missing_argument(self):
        # ./webserv should fail if no config file is passed
        code, out, err = run_and_capture(["./webserv"])
        self.assertEqual(code, 1)
        self.assertIn(b"Error: Usage: ./webserv <config_file>.conf", err)
        self.assertEqual(out, b"", "Expected no output to stdout")

    def test_01_empty_config_file(self):
        # It will test how the server handles an empty config file.
        with open(CONFIG_PATH, "w") as f:
            f.write("")
    
        code, out, err = run_and_capture(["./webserv", CONFIG_PATH])
        self.assertEqual(code, 1)
        self.assertIn(b"Error: Configuration file is empty.", err)
        self.assertEqual(out, b"", "Expected no output to stdout")

    def test_02_missing_server_name(self):
        # The config is missing the server_name field, which should be an error
        config = textwrap.dedent("""\
            server {
                host 127.0.0.1;
                listen 8080;
                root www/;
                client_max_body_size 3000000;
                index /index.html;
            }
        """)
        with open(CONFIG_PATH, "w") as f:
            f.write(config)

        code, out, err = run_and_capture(["./webserv", CONFIG_PATH])
        self.assertEqual(code, 1)
        self.assertIn(b"Error: Server name not set.", err)
        self.assertEqual(out, b"", "Expected no output to stdout")


    # -------------------------
    # TEMPLATE FOR NEW TESTS
    # -------------------------
    # def test_XX_short_description(self):
    #     """Explain briefly what this test checks."""
    #     config = textwrap.dedent(\"\"\"\
    #         server {
    #             server_name test;
    #             host 127.0.0.1;
    #             listen 8080;
    #             root www/;
    #         }
    #     \"\"\")
    #     with open(CONFIG_PATH, "w") as f:
    #         f.write(config)
    #
    #     code, out, err = run_and_capture(["./webserv", CONFIG_PATH])
    #     self.assertEqual(code, 0)  # Replace with expected return code
    #     self.assertIn(b"Expected output or error", out + err)

if __name__ == "__main__":
    unittest.main()
