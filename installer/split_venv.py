"""
Package sidecar/.venv into venv.zip using ZIP_STORED (no compression).
- Faster to create, faster to extract
- Files like .pyc .pyd .dll don't compress well anyway
- Inno Setup 6 ships it as-is with nocompression flag
Run from project root: python installer/split_venv.py
"""
import zipfile, pathlib, sys, time

venv    = pathlib.Path("sidecar/.venv").resolve()
out_dir = pathlib.Path("sidecar/dist")
out     = out_dir / "venv.zip"

if not venv.exists():
    print(f"[ERROR] venv not found at {venv}")
    sys.exit(1)

# Verify ace_step is installed in the venv
site_packages = venv / "Lib" / "site-packages"
ace_found = list(site_packages.glob("ace_step*")) + list(site_packages.glob("acestep*"))
if not ace_found:
    print(f"[ERROR] ace_step not found in venv site-packages!")
    print(f"        Run build_sidecar.bat again.")
    sys.exit(1)
print(f"  ACE-Step  : {[p.name for p in ace_found]}")

out_dir.mkdir(parents=True, exist_ok=True)

# Remove any old split parts
for old in out_dir.glob("venv_*.zip"):
    old.unlink()
    print(f"  Removed old {old.name}")

if out.exists():
    out.unlink()

files = sorted(f for f in venv.rglob("*") if f.is_file())
total_size = sum(f.stat().st_size for f in files)
print(f"  Files     : {len(files):,}")
print(f"  Total size: {total_size / 1e9:.2f} GB")
print(f"  Output    : {out}")
print(f"  Mode      : ZIP_STORED (no recompression - fast pack/unpack)")

t0 = time.time()
with zipfile.ZipFile(out, "w", zipfile.ZIP_STORED) as z:
    for i, f in enumerate(files):
        if i % 5000 == 0:
            print(f"  {i:>6}/{len(files)}...", flush=True)
        z.write(f, f.relative_to(venv))

elapsed = time.time() - t0
zip_size = out.stat().st_size
print(f"  venv.zip  : {zip_size / 1e9:.2f} GB  ({elapsed:.0f}s)")
print("  Done.")
