#!/usr/bin/env python3

import os
import sys

# Print HTTP headers
print("Content-Type: text/plain")
print()

# Environment variables
print("=== CGI Environment Variables ===")
for key, value in sorted(os.environ.items()):
    print(f"{key} = {value}")

# Read from stdin (for POST data)
print("\n=== CGI Standard Input (Request Body) ===")
if "CONTENT_LENGTH" in os.environ:
    try:
        content_length = int(os.environ["CONTENT_LENGTH"])
        body = sys.stdin.read(content_length)
        print(body)
    except Exception as e:
        print(f"Error reading body: {e}")
else:
    print("[No Content-Length set]")

# Path info and arguments
print("\n=== CGI Path Info and Arguments ===")
print(f"PATH_INFO = {os.environ.get('PATH_INFO', '')}")
print(f"SCRIPT_NAME = {os.environ.get('SCRIPT_NAME', '')}")


