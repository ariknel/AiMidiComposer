"""
Fast venv extraction using Python's zipfile module.
Called by the installer after copying venv.zip.
Usage: python extract_venv.py <zip_path> <dest_dir>
"""
import zipfile, pathlib, sys, time

if len(sys.argv) != 3:
    print("Usage: extract_venv.py <zip_path> <dest_dir>")
    sys.exit(1)

zip_path = pathlib.Path(sys.argv[1])
dest     = pathlib.Path(sys.argv[2])

if not zip_path.exists():
    print(f"[ERROR] {zip_path} not found")
    sys.exit(1)

dest.mkdir(parents=True, exist_ok=True)

print(f"Extracting {zip_path.name} -> {dest}", flush=True)
t0 = time.time()

with zipfile.ZipFile(zip_path, 'r') as z:
    members = z.namelist()
    total = len(members)
    for i, member in enumerate(members):
        if i % 5000 == 0:
            print(f"  {i}/{total}...", flush=True)
        z.extract(member, dest)

elapsed = time.time() - t0
print(f"Done. {total} files in {elapsed:.1f}s", flush=True)

zip_path.unlink()
print("Zip deleted.", flush=True)
