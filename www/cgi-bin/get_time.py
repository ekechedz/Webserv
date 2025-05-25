#!/usr/bin/env python3

import os
import cgi
import cgitb
from datetime import datetime

cgitb.enable()  # Enable debugging

print("Content-Type: text/html\n")  # HTTP header
print("<html><body>")
print("<h1>CGI Script Output</h1>")
print(f"<p>Current Server Time: {datetime.now()}</p>")

# Print environment variables
print("<h2>Environment Variables</h2>")
print("<ul>")
for key, value in os.environ.items():
    print(f"<li>{key}: {value}</li>")
print("</ul>")

# Handle POST data if present
form = cgi.FieldStorage()
if form:
    print("<h2>POST Data</h2>")
    print("<ul>")
    for key in form.keys():
        print(f"<li>{key}: {form.getvalue(key)}</li>")
    print("</ul>")

print("</body></html>")
