#!/usr/bin/env python3

import sys
import subprocess
import tempfile
import os

def main():
    # Read C code from stdin
    code = sys.stdin.read()

    if not code.strip():
        print("No input received.", file=sys.stderr)
        sys.exit(1)

    # Create temporary source file
    with tempfile.NamedTemporaryFile(mode="w", suffix=".c", delete=False) as src:
        src.write(code)
        src_name = src.name

    exe_name = src_name[:-2]

    try:
        # Compile
        compile_proc = subprocess.run(
            ["cc", src_name, "-o", exe_name],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        if compile_proc.returncode != 0:
            print(compile_proc.stderr, end="")
            sys.exit(compile_proc.returncode)

        # Run executable
        run_proc = subprocess.run(
            [exe_name],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        print(run_proc.stdout, end="")
        print(run_proc.stderr, end="", file=sys.stderr)

        sys.exit(run_proc.returncode)

    finally:
        # Cleanup
        try:
            os.remove(src_name)
        except OSError:
            pass

        try:
            os.remove(exe_name)
        except OSError:
            pass

if __name__ == "__main__":
    main()