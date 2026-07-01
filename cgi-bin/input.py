#!/usr/bin/env python3

import sys
import html

def main():
    # Read all input from stdin
    input_data = sys.stdin.read()
    
    # Escape HTML special characters
    escaped_data = html.escape(input_data)
    
    # Print as HTML
    print("HTTP/1.0 200 OK\r\n")
    print("Content-Type: text/html\r\n")
    print("Content-Lenght: 500\r\n")
    print("\r\n\r\n")
    print("<!DOCTYPE html>")
    print("<html>")
    print("<head>")
    print("    <title>Input Output</title>")
    print("    <meta charset=\"UTF-8\">")
    print("</head>")
    print("<body>")
    print("    <h1>Input Received</h1>")
    print("    <pre>")
    print(escaped_data)
    print("    </pre>")
    print("</body>")
    print("</html>")

if __name__ == "__main__":
    main()