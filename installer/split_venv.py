"""
Split sidecar/.venv into two zip archives that each stay under
Inno Setup 5's 2.1 GB single-file limit.
Run from the project root: python installer/split_venv.py
"""
import zipfile
import pathlib
import sys

venv  = pathlib.Path("sidecar/.venv").resolve()
out_a = pathlib.Path("sidecar/dist/venv_a.zip")
out_b = pathlib.Path("sidecar/dist/venv_b.zip")

if not venv.exists():
    print(f"[ERROR] venv not found at {venv}")
    sys.exit(1)

out_a.parent.mkdir(parents=True, exist_ok=True)

files = sorted(f for f in venv.rglob("*") if f.is_file())
mid   = len(files) // 2
part_a, part_b = files[:mid], files[mid:]

print(f"  Total files: {len(files)}")
print(f"  Part A: {len(part_a)} files -> {out_a.name}")
with zipfile.ZipFile(out_a, "w", zipfile.ZIP_DEFLATED, compresslevel=1) as z:
    for i, f in enumerate(part_a):
        if i % 1000 == 0:
            print(f"    {i}/{len(part_a)}...", flush=True)
        z.write(f, f.relative_to(venv))
print(f"  venv_a.zip: {out_a.stat().st_size:,} bytes")

print(f"  Part B: {len(part_b)} files -> {out_b.name}")
with zipfile.ZipFile(out_b, "w", zipfile.ZIP_DEFLATED, compresslevel=1) as z:
    for i, f in enumerate(part_b):
        if i % 1000 == 0:
            print(f"    {i}/{len(part_b)}...", flush=True)
        z.write(f, f.relative_to(venv))
print(f"  venv_b.zip: {out_b.stat().st_size:,} bytes")
print("  Done.")
