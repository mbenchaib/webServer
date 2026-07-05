#!/usr/bin/env python3
"""
CGI script that causes a segmentation fault for testing error handling
"""
import ctypes

# Attempt to dereference a null pointer to trigger SEGV
null_ptr = ctypes.CFUNCTYPE(ctypes.c_int)(0)
null_ptr()  # Call the null pointer -> SEGV
