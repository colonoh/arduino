#!/usr/bin/env python3
"""
Create flash image with index for W25Q128
Usage: python3 create_flash_image.py output.bin [folder]
       Processes all .mp3 files in folder (default: current directory)
"""

import sys
import struct
import subprocess
import os
from pathlib import Path

# Audio parameters
SAMPLE_RATE = 16000
CHANNELS = 1
FORMAT = 'u8'  # 8-bit unsigned PCM

def convert_to_raw(mp3_file):
    """Convert MP3 to raw PCM using ffmpeg"""
    raw_file = mp3_file.with_suffix('.raw')

    cmd = [
        'ffmpeg', '-y', '-i', str(mp3_file),
        '-ar', str(SAMPLE_RATE),
        '-ac', str(CHANNELS),
        '-f', FORMAT,
        str(raw_file)
    ]

    print(f"  Converting {mp3_file.name} -> {raw_file.name}")
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)

    return raw_file

def create_flash_image(output_file, input_files):
    """Create flash image with index and audio data"""

    # Convert all MP3s to raw PCM
    print(f"Converting {len(input_files)} files to raw PCM...")
    raw_files = []
    for mp3 in input_files:
        if not mp3.exists():
            print(f"Error: File not found: {mp3}")
            sys.exit(1)
        raw_files.append(convert_to_raw(mp3))

    print("\nBuilding index...")

    # Calculate header size: 4 (magic) + 2 (count) + 8*n (table)
    num_tracks = len(raw_files)
    header_size = 6 + num_tracks * 8

    # Build index in memory
    index_data = bytearray()

    # Magic number "AUDI"
    index_data.extend(b'AUDI')

    # Track count (uint16, little-endian)
    index_data.extend(struct.pack('<H', num_tracks))

    # Build track table
    print("Track table:")
    print("  Idx  Offset      Length      Duration   Filename")
    print("  ---  ----------  ----------  ---------  --------")

    current_offset = header_size
    for i, raw_file in enumerate(raw_files):
        size = raw_file.stat().st_size
        duration = size / SAMPLE_RATE  # seconds for 8-bit mono

        # Write offset and length (uint32, little-endian)
        index_data.extend(struct.pack('<I', current_offset))
        index_data.extend(struct.pack('<I', size))

        print(f"  {i:3d}  0x{current_offset:08x}  0x{size:08x}  {duration:6.1f}s    {raw_file.name}")

        current_offset += size

    # Write flash image
    print(f"\nCreating flash image: {output_file}")
    with open(output_file, 'wb') as out:
        # Write index
        out.write(index_data)

        # Append all raw audio data
        for raw_file in raw_files:
            with open(raw_file, 'rb') as f:
                out.write(f.read())

    # Show final size
    final_size = os.path.getsize(output_file)
    w25q128_size = 16 * 1024 * 1024  # 16 MB

    print("\nFlash image created successfully!")
    print(f"  Total size: {final_size:,} bytes ({final_size // 1024} KB)")
    print(f"  W25Q128 capacity: {w25q128_size:,} bytes (16 MB)")
    print(f"  Usage: {final_size / w25q128_size * 100:.1f}%")
    print(f"\nTo flash:")
    print(f"  flashrom -p <programmer> -w {output_file}")
    print(f"  or: minipro -p W25Q128@DIP8 -w {output_file}")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python3 create_flash_image.py output.bin [folder]")
        print("       Processes all .mp3 files in folder (default: current directory)")
        sys.exit(1)

    output = Path(sys.argv[1])
    folder = Path(sys.argv[2]) if len(sys.argv) > 2 else Path('.')

    if not folder.is_dir():
        print(f"Error: {folder} is not a directory")
        sys.exit(1)

    # Find all MP3 files and sort them alphabetically
    inputs = sorted(folder.glob('*.mp3'))

    if len(inputs) == 0:
        print(f"Error: No .mp3 files found in {folder}")
        sys.exit(1)

    if len(inputs) > 65535:
        print("Error: Maximum 65535 tracks supported")
        sys.exit(1)

    print(f"Found {len(inputs)} MP3 file(s) in {folder}")
    create_flash_image(output, inputs)
