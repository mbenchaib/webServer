#!/usr/bin/env python3
"""
CGI script that throws an unhandled exception
"""
import os

print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n")
print("About to crash with exception...")

# Cause an exception
result = 1 / 0  # ZeroDivisionError
