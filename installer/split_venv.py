"""
Split sidecar/.venv into zip archives each under 1.8 GB.
Splits by cumulative file size, not file count, so large .pth files
(torch weights) are distributed evenly.
Run from project root: python installer/split_venv.py
"""
import zipfile
import pathlib
import sys

MAX_BYTES = 1_800_000_000  # 1.8 GB per zip - safely under Inno Setup 5's 2.1 GB limit

venv    = pathlib.Path("sidecar/.venv").resolve()
out_dir = pathlib.Path("sidecar/dist")

if not venv.exists():
    print(f"[ERROR] venv not found at {venv}")
    sys.exit(1)

out_dir.mkdir(parents=True, exist_ok=True)

# Delete any existing venv_*.zip parts
for old in out_dir.glob("venv_*.zip"):
    old.unlink()

# Sort files largest-first so big files are spread across parts
files = sorted(
    (f for f in venv.rglob("*") if f.is_file()),
    key=lambda f: f.stat().st_size,
    reverse=True,
)

print(f"  Total files : {len(files)}")
print(f"  Total size  : {sum(f.stat().st_size for f in files):,} bytes")
print(f"  Max per zip : {MAX_BYTES:,} bytes")

# Greedy bin-packing: assign each file to the part with most remaining space
# (ensures large torch .pth files each go into a different part)
parts = []  # list of (current_size, [files])

for f in files:
    size = f.stat().st_size
    # Find a part with enough room, or start a new one
    placed = False
    for part in parts:
        if part[0] + size <= MAX_BYTES:
            part[1].append(f)
            part[0] += size
            placed = True
            break
    if not placed:
        parts.append([size, [f]])

print(f"  Parts needed: {len(parts)}")

for idx, (part_size, part_files) in enumerate(parts):
    label = chr(ord('a') + idx)
    out   = out_dir / f"venv_{label}.zip"
    print(f"  Part {label.upper()}: {len(part_files)} files, {part_size:,} bytes -> {out.name}")
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=1) as z:
        for i, f in enumerate(part_files):
            if i % 2000 == 0:
                print(f"    {i}/{len(part_files)}...", flush=True)
            z.write(f, f.relative_to(venv))
    actual = out.stat().st_size
    print(f"  {out.name}: {actual:,} bytes")
    if actual > 2_100_000_000:
        print(f"  [ERROR] {out.name} exceeds Inno Setup 5 limit!")
        sys.exit(1)

print("  Done.")
