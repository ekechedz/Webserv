#!/usr/bin/python3
import os
import sys

print("Content-Type: text/html\n")
print("<html><body>")
print("<h1>CGI Test</h1>")
print("<p>REQUEST_METHOD: {}</p>".format(os.environ.get("REQUEST_METHOD", "")))
print("<p>QUERY_STRING: {}</p>".format(os.environ.get("QUERY_STRING", "")))
print("<p>CONTENT_TYPE: {}</p>".format(os.environ.get("CONTENT_TYPE", "")))
print("<p>CONTENT_LENGTH: {}</p>".format(os.environ.get("CONTENT_LENGTH", "")))
if os.environ.get("REQUEST_METHOD", "") == "POST":
    body = sys.stdin.read(int(os.environ.get("CONTENT_LENGTH", "0")))
    print("<p>POST Body: {}</p>".format(body))
print("</body></html>")