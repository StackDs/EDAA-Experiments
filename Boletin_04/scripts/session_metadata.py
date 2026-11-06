#!/usr/bin/env python3
"""Registra y valida metadatos sin ejecutar los experimentos."""

import argparse
from datetime import datetime, timezone
import hashlib
import importlib.metadata
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

BASE = Path(__file__).resolve().parents[1]


def positive(text):
    value = int(text)
    if value <= 0 or value > (1 << 64) - 1:
        raise ValueError("Entero positivo fuera de rango")
    return value


def numbers(text):
    values = [positive(item) for item in text.split(",")]
    if len(values) != len(set(values)):
        raise ValueError("Valores duplicados")
    return values


def prepare():
    fields = {"sizes": "SIZES", "seeds": "SEEDS", "cycles": "CYCLES",
              "dynamic_queries": "DYNAMIC_QUERIES", "static_queries": "STATIC_QUERIES",
              "target_ns": "TARGET_NS", "max_batch": "MAX_BATCH", "cpu_core": "CPU_CORE"}
    protocol = {key: numbers(os.environ[env]) if key in ["sizes", "seeds"] else positive(os.environ[env])
                for key, env in fields.items() if key != "cpu_core"}
    protocol["cpu_core"] = int(os.environ["CPU_CORE"])
    profile = os.environ["PROFILE"]
    runs = int(os.environ["RUNS"])
    if profile not in ["pilot", "official", "smoke"] or runs != (32 if profile == "official" else 4):
        raise ValueError("Perfil o repeticiones invalidos")
    if (max(protocol["sizes"]) > 1000000 or protocol["cycles"] > 100
            or protocol["static_queries"] > protocol["max_batch"]
            or protocol["max_batch"] > 1000000 or protocol["dynamic_queries"] > 10000):
        raise ValueError("Configuracion fuera de los limites admitidos")
    for n in protocol["sizes"]:
        lengths = {k for k in [1, 8, 63, 511, max(1, n // 2), n] if k <= n}
        if protocol["dynamic_queries"] < len(lengths):
            raise ValueError("Faltan consultas para repartir entre las longitudes")
    protocol.update(version="rmq-v1", official_runs=32, entry="pair<int64_t,uint64_t>",
                    ranges=["1", "8", "63", "511", "half", "full"],
                    generator="mt19937_64-rejection-v1", quartiles="uhr")
    timeout = positive(os.environ["TIMEOUT_SECONDS"])
    if timeout > 3600:
        raise ValueError("El presupuesto no puede superar una hora")
    receipt = os.environ.get("SESSION_PATH_FILE", "")
    if receipt:
        receipt = str(Path(receipt).expanduser().resolve())
        if Path(receipt).exists() or not Path(receipt).parent.is_dir():
            raise ValueError("La ruta de salida de la sesion debe ser nueva y tener un directorio existente")
    paths = sorted({*BASE.glob("include/**/*.hpp"), *BASE.glob("src/*.cpp"), *BASE.glob("benchmarks/*.sh"),
                    *BASE.glob("scripts/*.py"), *BASE.glob("plots/*.py"), BASE / "Makefile"})
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    sources = {str(path.relative_to(BASE)): digest(path) for path in paths}
    binaries = {name: digest(BASE / "bin" / name) for name in ["bench_rmq", "bench_memory"]}
    build = json.loads((BASE / "bin/build-config.json").read_text())
    cpuinfo = Path("/proc/cpuinfo").read_text().split("\n\n")[0]
    cpu = {line.split(":", 1)[0].strip(): line.split(":", 1)[1].strip()
           for line in cpuinfo.splitlines() if ":" in line}
    runtime = {"hostname": platform.node(), "platform": platform.platform(), "machine": platform.machine(),
               "python": sys.version, "libc": platform.libc_ver(),
               "packages": {name: importlib.metadata.version(name) for name in ["numpy", "pandas", "matplotlib"]},
               "cpu": {name: cpu.get(name) for name in ["vendor_id", "cpu family", "model", "model name", "stepping", "microcode"]}}
    signature = hashlib.sha256(json.dumps([protocol, sources, binaries, build, runtime], sort_keys=True).encode()).hexdigest()
    if profile == "official":
        pilot_path = Path(os.environ["PILOT"]).expanduser().resolve()
        pilot = json.loads((pilot_path / "session.json").read_text())
        if not (pilot_path / "COMPLETE").is_file() or pilot.get("profile") != "pilot":
            raise ValueError("El piloto no esta completo")
        if pilot.get("config_id") != signature:
            raise ValueError("El piloto pertenece a otra configuracion compilacion o fuentes")
        if not pilot.get("budget_approved"):
            raise ValueError("La proyeccion del piloto supera el objetivo de 45 minutos")
        timeout = min(timeout, max(1, int(3600 - pilot["elapsed_seconds"])))
    metadata = dict(protocol=protocol, config_id=signature, profile=profile, runs=runs,
                    sources=sources, binaries=binaries, build=build, runtime=runtime,
                    platform=platform.platform(), machine=platform.machine(),
                    cpu_affinity=sorted(os.sched_getaffinity(0)), cpuinfo=cpuinfo)
    metadata["git_revision"] = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=BASE, text=True).strip()
    metadata["git_status"] = subprocess.check_output(["git", "status", "--short", "--", "."], cwd=BASE, text=True)
    Path(os.environ["CONTEXT_FILE"]).write_text(json.dumps({"metadata": metadata, "receipt": receipt}) + "\n")
    print(signature, timeout, ",".join(map(str, protocol["sizes"])), ",".join(map(str, protocol["seeds"])),
          protocol["cycles"], protocol["dynamic_queries"], protocol["static_queries"],
          protocol["target_ns"], protocol["max_batch"])


def record(action):
    directory = Path(os.environ["SESSION_DIR"])
    manifest = directory / "session.json"
    if action == "begin":
        metadata = json.loads(Path(os.environ["CONTEXT_FILE"]).read_text())["metadata"]
        metadata.update(session_id=directory.name, created=datetime.now(timezone.utc).isoformat())
    else:
        if action == "fail" and not manifest.exists():
            return
        metadata = json.loads(manifest.read_text())
        elapsed = int(os.environ["ELAPSED_CS"]) / 100
        metadata["elapsed_seconds"] = elapsed
        if action == "finish":
            if metadata["profile"] == "pilot":
                projected = elapsed * (1 + 1.25 * 32 / metadata["runs"])
                metadata.update(projected_total_seconds=projected, budget_approved=projected <= 2700)
                print(f"Proyeccion conservadora con piloto incluido: {projected / 60:.1f} minutos")
            metadata["status"] = "complete"
            receipt = json.loads(Path(os.environ["CONTEXT_FILE"]).read_text())["receipt"]
            if receipt:
                Path(receipt).write_text(str(directory) + "\n")
        else:
            metadata.update(status="incomplete", error=os.environ.get("ERROR_MESSAGE", "Ejecucion interrumpida"))
            (directory / "COMPLETE").unlink(missing_ok=True)
    manifest.write_text(json.dumps(metadata, indent=2) + "\n")
    if action == "finish":
        (directory / "COMPLETE").write_text(metadata["config_id"] + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["prepare", "begin", "finish", "fail"])
    args = parser.parse_args()
    if args.action == "prepare":
        prepare()
    else:
        record(args.action)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, importlib.metadata.PackageNotFoundError, subprocess.SubprocessError) as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)
