#!/usr/bin/env bash
#
# genicon.sh - generate the ADS Explorer overlay icon (src/res/overlay.ico).
#
# Writes a 16x16 / 32bpp ICO as a raw byte stream, so it needs no image
# toolchain and no Python.
#
# Design: a purple skull (danger / caution). It is a filled pixel drawing
#         rather than a line symbol, so it does not get confused with the
#         symbol-style overlays used by TortoiseGit/SVN.
#
# Edit rows[] below to change the artwork (one character = one pixel):
#   .  transparent    P  purple (body)    W  white (eyes / nose / teeth)
# To change the colors, edit the BGRA values in COL_* below.
#
# Regenerate with:  bash tools/genicon.sh
#
set -euo pipefail
export LANG=C

OUT_DIR="$(cd "$(dirname "$0")/.." && pwd)/src/res"
OUT="$OUT_DIR/overlay.ico"
mkdir -p "$OUT_DIR"

# ---- Colors (BGRA) --------------------------------------------------------
COL_P='\xb0\x27\x9c\xff'   # purple #9C27B0
COL_W='\xff\xff\xff\xff'   # white
COL_T='\x00\x00\x00\x00'   # transparent

# ---- Pixel art (top to bottom, y0..y15, 16 characters per row) ------------
rows=(
"................"
"....PPPPPPPP...."
"...PPPPPPPPPP..."
"..PPPPPPPPPPPP.."
"..PPPPPPPPPPPP.."
"..PWWPPPPPPWWP.."
"..PWWPPPPPPWWP.."
"..PPPPPPPPPPPP.."
"..PPPPPWWPPPPP.."
"...PPPPPPPPPP..."
"....PPPPPPPP...."
"....PPPPPPPP...."
"....PWPWPWPW...."
"....PPPPPPPP...."
"................"
"................"
)

# ---- Headers --------------------------------------------------------------
# ICONDIR
data='\x00\x00\x01\x00\x01\x00'
# ICONDIRENTRY: width=16, height=16, colors=0, reserved=0, planes=1, bpp=32
data+='\x10\x10\x00\x00\x01\x00\x20\x00'
# dwBytesInRes = 40 + 1024 + 64 = 1128 = 0x468
data+='\x68\x04\x00\x00'
# dwImageOffset = 22 = 0x16
data+='\x16\x00\x00\x00'
# BITMAPINFOHEADER
data+='\x28\x00\x00\x00'  # biSize = 40
data+='\x10\x00\x00\x00'  # biWidth = 16
data+='\x20\x00\x00\x00'  # biHeight = 32 (XOR + AND)
data+='\x01\x00\x20\x00'  # biPlanes = 1, biBitCount = 32
data+='\x00\x00\x00\x00'  # biCompression = BI_RGB
data+='\x00\x04\x00\x00'  # biSizeImage = 1024
data+='\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

# ---- XOR pixels (BGRA, bottom-up: y15..y0) --------------------------------
for ((r=15; r>=0; r--)); do
  line="${rows[r]}"
  for ((x=0; x<16; x++)); do
    case "${line:x:1}" in
      P) data+="$COL_P" ;;
      W) data+="$COL_W" ;;
      *) data+="$COL_T" ;;
    esac
  done
done

# ---- AND mask (all zero; 32bpp uses the alpha channel instead) ------------
for ((i=0; i<64; i++)); do data+='\x00'; done

printf '%b' "$data" > "$OUT"
echo "generated: $OUT ($(wc -c < "$OUT") bytes / expected 1150)"
