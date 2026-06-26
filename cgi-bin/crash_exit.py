#!/usr/bin/env python3
"""
CGI script that exits with error code
"""
import sys

print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n")
print("About to exit with error code...")
sys.exit(127)  # Exit with error code
