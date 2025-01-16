import argparse
import math
import struct
import sys
import traceback
from dataclasses import dataclass

"""
Script to convert the ASCII BSC5 catalogue to it's binary representation.

Sources:
    - Gzipped ASCII: http://tdc-www.harvard.edu/catalogs/ybsc5.gz
        (SHA-256 unzipped: 69797549cc1605aad7ff94e9325e29a1661f2a253917faaa056d9bf20b809afd)
    - Binary: http://tdc-www.harvard.edu/catalogs/BSC5
        (SHA-256: e471d02eaf4eecb61c12f879a1cb6432ba9d7b68a9a8c5654a1eb42a0c8cc340)

Formats
    - ASCII: http://tdc-www.harvard.edu/catalogs/bsc5.entry.html
    - Binary:
        - Header: http://tdc-www.harvard.edu/catalogs/bsc5.header.html
        - Entry: http://tdc-www.harvard.edu/catalogs/bsc5.entry.html


NOTE: While this script does not produce a byte-for-byte equivalent of the
online binary BSC5 due to floating-point precision variations, it is effectively
just as accurate. Both the online binary BSC5 and this script derive their data
from the same ASCII source. Even changing the grouping of operations can affect
the float value output due to the non-associative nature of floating-point
arithmetic!

You can verify the generated file is very similar to the online version by
performing the following:

```
python3 scripts/bsc5_ascii_to_bin.py -i ybsc5 -o bsc5_gen
xxd bsc5 > bsc5.hex
xxd bsc5_gen > bsc5_gen.hex
git diff --no-index --word-diff=color --word-diff-regex=. bsc5.hex bsc5_gen.hex
```

You should find that almost all differences lie in the least two significant
bytes.
"""

# Constants for binary file header
STAR0 = 0
STAR1 = 1
STARN = -9110 # Negative represents J2000
STNUM = 1
MPROP = 1
NMAG = 1 # This is positive 1 for some reason, assuming the the comment in the spec is a typo
NBENT = 32

# Header (28) + 9110 * Entries (32)
BSC5_SIZE = 291548

# Use the J2000 Epoch
@dataclass
class CatalogEntry:
    XNO: float         # Catalog number of star (Real*4)
    SRA0: float        # B1950 Right Ascension in radians (Real*8)
    SDEC0: float       # B1950 Declination in radians (Real*8)
    IS: str            # Spectral type (Character*2)
    MAG: int           # V Magnitude * 100 (Integer*2)
    XRPM: float        # R.A. proper motion in radians per year (Real*4)
    XDPM: float        # Dec. proper motion in radians per year (Real*4)

# First 28 bytes of binary file
# Obtained using `xxd -l 28 -p bsc5_file`
EXPECTED_HEADER : bytes = b'\x00\x00\x00\x00\x01\x00\x00\x00\x6a\xdc\xff\xff' \
                          b'\x01\x00\x00\x00\x01\x00\x00\x00\x01\x00\x00\x00' \
                          b'\x20\x00\x00\x00'

# First 32 byte entry of binary file
# Obtained using `xxd -s 28 -l 32 -p bsc5_file`
EXPECTED_FIRST_ENTRY: bytes = b'\x00\x00\x80\x3f\x75\x98\xcc\x34\xd3\x13\x97\x3f' \
                               b'\xe6\x19\xc3\x55\xbf\x42\xe9\x3f\x41\x31\x9e\x02' \
                               b'\xfe\xde\x79\xb3\x3f\x67\xbb\xb3'

def parse_int(string : str) -> int:
    if string.strip():
        return int(string)
    else:
        return 0

def parse_float(string : str) -> int:
    if string.strip():
        return float(string)
    else:
        return 0.0

def parse_ascii_line(line: str) -> CatalogEntry:
    """Parse an ASCII line and return a CatalogEntry object."""

    # Catalog number (Real*4)
    XNO = float(line[0:4])

    # Right Ascension (RA) and Declination (Dec) in radians
    RAh = parse_int(line[75:77])
    RAm = parse_int(line[77:79])
    RAs = parse_float(line[79:83])

    SRA0 = (RAh + RAm / 60 + RAs / 3600) * (math.pi / 12)

    DE_sign = line[83]
    DEd = parse_int(line[84:86])
    DEm = parse_int(line[86:88])
    DEs = parse_float(line[88:90])

    Dec_degrees = DEd + DEm / 60 + DEs / 3600
    if DE_sign == '-':
        Dec_degrees = -Dec_degrees
    SDEC0 = Dec_degrees * (math.pi / 180)

    # Spectral type (Character*2)
    IS = line[129:131]

    MAG = int(parse_float(line[102:107]) * 100)

    # Proper motion in R.A. and Dec. (radians/year)
    pmRA = parse_float(line[148:154])
    pmDE = parse_float(line[154:160])
    XRPM = pmRA * (math.pi / (180 * 3600))
    XDPM = pmDE * (math.pi / (180 * 3600))

    # Return a CatalogEntry
    return CatalogEntry(XNO=XNO, SRA0=SRA0, SDEC0=SDEC0, IS=IS, MAG=MAG, XRPM=XRPM, XDPM=XDPM)

def decimal_to_integer4_little_endian(value):
    return value.to_bytes(4, byteorder="little", signed=True)

def create_binary_header() -> bytes:
    """Write the binary file header."""
    header = (
        decimal_to_integer4_little_endian(STAR0) +
        decimal_to_integer4_little_endian(STAR1) +
        decimal_to_integer4_little_endian(STARN) +
        decimal_to_integer4_little_endian(STNUM) +
        decimal_to_integer4_little_endian(MPROP) +
        decimal_to_integer4_little_endian(NMAG) +
        decimal_to_integer4_little_endian(NBENT)
    )
    assert header == EXPECTED_HEADER, (
            f"Binary entry does not match the expected value.\n"
            f"Expected:\t {EXPECTED_HEADER.hex()}\n"
            f"Found:\t\t {header.hex()}"
    )
    return header


def create_binary_entry(entry : CatalogEntry) -> bytes:
    """Write a single catalog entry to the binary file."""
    binary_entry = (
        struct.pack('f', entry.XNO) +                # Catalog number (Real*4)
        struct.pack('d', entry.SRA0) +               # Right Ascension in radians (Real*8)
        struct.pack('d', entry.SDEC0) +              # Declination in radians (Real*8)
        entry.IS.encode('ascii').ljust(2, b'\x00') + # Spectral type (Character*2)
        struct.pack('h', entry.MAG) +                # V Magnitude * 100 (Integer*2)
        struct.pack('f', entry.XRPM) +               # R.A. proper motion (Real*4)
        struct.pack('f', entry.XDPM)                 # Dec. proper motion (Real*4)
    )

    assert len(binary_entry) == 32, (
            f"Binary entry length is not 32 bytes for entry {entry.XNO}.\n"
    )

    return binary_entry

def ascii_to_binary(ascii_file, binary_file):
    """Convert the ASCII catalogue to binary format."""
    with open(ascii_file, 'r') as infile, open(binary_file, 'wb') as outfile:

        header = create_binary_header()
        outfile.write(header)

        lines = infile.readlines()
        previous_entry = 0
        for line in lines:
            entry = parse_ascii_line(line)
            previous_entry = entry.XNO

            entry_bin = create_binary_entry(entry)
            outfile.write(entry_bin)

def file_byte_sum(file_path):
    total_sum = 0
    with open(file_path, 'rb') as file:
        # Read in chunks for efficiency
        while chunk := file.read(4096):
            total_sum += sum(chunk)
    return total_sum

def main():
    parser = argparse.ArgumentParser(description="Convert ASCII catalogue to binary format.")
    parser.add_argument('-i', '--input', required=True, help="Input ASCII file")
    parser.add_argument('-o', '--output', required=True, help="Output binary file")
    args = parser.parse_args()

    try:

        ascii_to_binary(args.input, args.output)
        with open(args.output, "rb") as file:
            file.seek(0, 2)
            file_size = file.tell()

        assert file_size == BSC5_SIZE, (
            f"Binary file size does not match the expected value.\n"
            f"Expected:\t {BSC5_SIZE}\n"
            f"Found:\t\t {file_size}"
        )

        # Ensure hex sums of two files are relatively close as a rough
        # way to ensure data is similar
        generated_sum = file_byte_sum(args.output)
        bsc5_sum = 32911474
        max_dist = 128
        assert abs(generated_sum - bsc5_sum) <= max_dist, (
            f"Generated file checksum verification failed:\n"
            f"- Generated file checksum: {generated_sum}\n"
            f"- Expected checksum: {bsc5_sum}\n"
            f"- Allowed difference: {max_dist}\n"
            f"- Actual difference: {abs(generated_sum - bsc5_sum)}"
        )

    except AssertionError as e:
        print(f"Assertion failed: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"An unexpected error occurred: {e}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
