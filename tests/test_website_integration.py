#!/usr/bin/env python3

import unittest
import subprocess
import requests
import time
import os
import re
import signal

CONFIG_PATH = "config_file/default.conf"

# Extract the host and port from the server config file
def extract_host_and_port(config_path):
    with open(config_path, "r") as f:
        content = f.read()
    port_match = re.search(r"listen\s+(\d+);", content)
    host_match = re.search(r"host\s+([\d.]+);", content)
    if not port_match or not host_match:
        raise ValueError("Could not find host or port in config")
    return host_match.group(1), int(port_match.group(1))

class WebsiteIntegrationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Read host/port from config and form the test URL
        cls.host, cls.port = extract_host_and_port(CONFIG_PATH)
        cls.base_url = f"http://{cls.host}:{cls.port}"

        # Launch the webserv process, suppressing output
        cls.server = subprocess.Popen(
            ["./webserv", CONFIG_PATH],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )

        # Wait briefly for server to start, then test connection
        try:
            time.sleep(1)
            requests.get(cls.base_url, timeout=2)
        except Exception:
            cls.tearDownClass()
            raise RuntimeError("Server did not start properly")

    @classmethod
    def tearDownClass(cls):
        # Cleanly terminate server process
        if cls.server.poll() is None:
            cls.server.terminate()
            try:
                cls.server.wait(timeout=2)
            except subprocess.TimeoutExpired:
                cls.server.kill()

    # Utility method to verify that a given path returns the expected static file
    def assertResponseMatchesFile(self, path, expected_file):
        try:
            res = requests.get(self.base_url + path, timeout=2)
        except requests.exceptions.Timeout:
            self.fail("Request timed out (possible infinite loop)")
        self.assertEqual(res.status_code, 200)
        with open(expected_file, "rb") as f:
            expected = f.read()
        self.assertEqual(res.content.strip(), expected.strip())

    # Tests -----------------------------------------------------------------

    def test_01_root_path_returns_index(self):
        self.assertResponseMatchesFile("/", "www/index.html")

    def test_02_index_path_returns_index(self):
        self.assertResponseMatchesFile("/index.html", "www/index.html")

    def test_03_generic_path_returns_generic(self):
        self.assertResponseMatchesFile("/generic.html", "www/generic.html")

    # Template for adding more tests ---------------------------------------
    # def test_XX_description(self):
    #     """Short explanation of what this test checks"""
    #     self.assertResponseMatchesFile("/path", "www/expected_file.html")

if __name__ == "__main__":
    unittest.main()
