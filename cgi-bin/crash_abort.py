#!/usr/bin/env python3
"""
CGI script that calls os.abort() to terminate with SIGABRT
"""
import os

print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n")
print("About to call os.abort()...")
os.abort()  # Terminate with SIGABRT signal
