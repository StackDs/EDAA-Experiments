#!/usr/bin/env python3
import argparse
import gzip
import hashlib
import json
import os
import shutil
import tempfile
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

URL = "https://pizzachili.dcc.uchile.cl/texts/nlang/english.50MB.gz"
SIZE = 10_000_000
SHA256 = "ab426bd5952843c190cc9705f94dfe42a1fbbc53dcb06f480ebd21facea6d9f7"


def digest(path):
    with Path(path).open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def prepare(directory, source=None, size=SIZE, expected=SHA256):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    target = directory / "english.bin"
    archive = directory / "english.50MB.gz"
    manifest = directory / "manifest.json"
    if target.exists() and (target.stat().st_size != size or digest(target) != expected):
        raise ValueError("El texto existente no coincide con el corpus esperado")

    # Preparamos archivos temporales sin reemplazar datos validos
    with tempfile.TemporaryDirectory(dir=directory, prefix=".prepare-") as temporary:
        temp = Path(temporary)
        candidate = temp / archive.name
        if archive.exists():
            candidate = archive
        elif source is not None:
            shutil.copyfile(source, candidate)
        else:
            with urllib.request.urlopen(URL, timeout=120) as response, candidate.open("wb") as output:
                shutil.copyfileobj(response, output)
        prefix = temp / target.name
        total = 0
        # Leemos hasta el final para validar tambien el CRC del gzip
        with gzip.open(candidate, "rb") as compressed, prefix.open("wb") as output:
            while chunk := compressed.read(1 << 20):
                remaining = max(0, size - total)
                output.write(chunk[:remaining])
                total += len(chunk)
        if total < size or digest(prefix) != expected:
            raise ValueError("El corpus esta incompleto o su SHA256 no coincide")
        metadata = {
            "url": URL, "retrieved_utc": datetime.now(timezone.utc).isoformat(),
            "archive": archive.name, "archive_bytes": candidate.stat().st_size,
            "archive_sha256": digest(candidate), "decompressed_bytes": total,
            "text": target.name, "text_bytes": size, "text_sha256": expected,
            "mb_bytes": 1_000_000, "encoding": "raw bytes",
        }
        if manifest.exists():
            old = json.loads(manifest.read_text())
            if any(old.get(key) != metadata[key] for key in
                   ("archive_sha256", "text_sha256", "text_bytes")):
                raise ValueError("El manifiesto existente no coincide")
        if not archive.exists():
            os.replace(candidate, archive)
        if not target.exists():
            os.replace(prefix, target)
        if not manifest.exists():
            entry = temp / manifest.name
            entry.write_text(json.dumps(metadata, indent=2) + "\n")
            os.replace(entry, manifest)
    return target


def main():
    parser = argparse.ArgumentParser(description="Prepara el corpus original de Pizza y Chili")
    parser.add_argument("--directory", type=Path, default=Path(__file__).resolve().parents[1] / "texts")
    parser.add_argument("--source", type=Path)
    args = parser.parse_args()
    print(prepare(args.directory, args.source))


if __name__ == "__main__":
    main()
