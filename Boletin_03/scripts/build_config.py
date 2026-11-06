#!/usr/bin/env python3
import argparse
import json
import shlex
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--cxx", required=True)
parser.add_argument("--flags", required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
compiler = subprocess.check_output(shlex.split(args.cxx) + ["--version"], text=True).splitlines()[0]
contents = json.dumps({"compiler_command": args.cxx, "compiler": compiler, "flags": args.flags}, indent=2) + "\n"
# Actualizamos la marca solo cuando cambia la configuracion de compilacion
if not args.output.exists() or args.output.read_text() != contents:
    args.output.write_text(contents)
