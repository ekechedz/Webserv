#!/usr/bin/python3

import os
import sys

print("Content-Type: text/html")
print()

print("<html><body>")
print("<h1>Hello from CGI!</h1>")
print("<p>Request method: {}</p>".format(os.environ.get("REQUEST_METHOD", "N/A")))
print("<p>Path Info: {}</p>".format(os.environ.get("PATH_INFO", "N/A")))
print("</body></html>")
