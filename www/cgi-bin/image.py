#!/usr/bin/python3

import cgi
import sys
import os

first_name = "FUCKING"
last_name = "NIGGER"
# print(os.environ["QUERY_STRING"], file=sys.stderr)
print("HTTP/1.1 200 OK")
print("Content-type: text/html\r\n\r\n")
print("<html>")
print("<head>")
print("<title>Hello - Second CGI Program</title>")
print("<html>")
print("<head>")
print("<h2>Hello %s %s</h2>" % (first_name, last_name))
print("</body>")
print("</html>")