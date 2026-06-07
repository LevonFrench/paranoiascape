#!/usr/bin/env python3
"""
Extract Paranoiascape AVI videos to raw RGB555 frame sequences.

Output format: <name>.rgb555 — concatenated 320*240*2 byte frames (little-endian RGB555).
First 8 bytes of each file is a header: uint32 width, uint32 height.
Remaining bytes are raw frame data (width * height * 2 bytes per frame).

Usage:
    python extract_videos.py [--hd]  # --hd for upscaled versions (future)
"""
import subprocess, struct, sys, os

SRC_DIR = os.path.join(os.path.dirname(__file__), "..", "isos", "extracted", "STR")
SD_DIR  = os.path.join(os.path.dirname(__file__), "build", "runner", "videos", "sd")
HD_DIR  = os.path.join(os.path.dirname(__file__), "build", "runner", "videos", "hd")

VIDEOS = ["LOGO", "MOVTEST", "PARAFNMV", "SMG"]

def extract(name, src_dir, dst_dir, width=320, height=240):
    avi = os.path.join(src_dir, f"{name}.avi")
    out = os.path.join(dst_dir, f"{name}.rgb555")
    
    if not os.path.exists(avi):
        print(f"[SKIP] {avi} not found")
        return
    
    os.makedirs(dst_dir, exist_ok=True)
    
    # Extract raw bgr555le frames via ffmpeg pipe
    print(f"[EXTRACT] {name}.avi -> {name}.rgb555 ({width}x{height})")
    cmd = [
        "ffmpeg", "-y", "-i", avi,
        "-f", "rawvideo", "-pix_fmt", "bgr555le",
        "-s", f"{width}x{height}",
        "pipe:1"
    ]
    result = subprocess.run(cmd, capture_output=True)
    if result.returncode != 0:
        print(f"[ERROR] ffmpeg failed: {result.stderr.decode()[:200]}")
        return
    
    raw = result.stdout
    frame_size = width * height * 2
    n_frames = len(raw) // frame_size
    
    # Write with header
    with open(out, "wb") as f:
        f.write(struct.pack("<II", width, height))
        f.write(raw[:n_frames * frame_size])
    
    print(f"  -> {n_frames} frames, {os.path.getsize(out) / 1024 / 1024:.1f} MB")

if __name__ == "__main__":
    use_hd = "--hd" in sys.argv
    dst = HD_DIR if use_hd else SD_DIR
    
    for name in VIDEOS:
        extract(name, SRC_DIR, dst)
    
    print("\nDone. Place upscaled AVIs in isos/extracted/STR/ and re-run with --hd")
