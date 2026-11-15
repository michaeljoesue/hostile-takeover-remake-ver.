#!/usr/bin/env python3
"""Updates the text data inside game/htdata832.pdb without rebuilding the art.

Recompiles the .ini files (forms.ini, game.ini, GobTemplates.ini) from their
.ini.pp sources and replaces them, help.txt, version.txt and the fonts in
data/fonts in the existing game/htdata832.pdb. Use this after editing menus,
unit stats, help text or fonts; it doesn't need the .NET content tools, only inicrunch.exe.

Usage (from any directory):

    python data\\update_text.py [--inicrunch PATH] [--pdb PATH]

To change art or levels, use build.py, which rebuilds everything.
"""

import argparse
import os
import struct
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build  # noqa: E402

# A .pdb is a Palm database: a 78 byte header, a table of 8 byte record
# entries, then the records. Record 0 is the directory: 32 byte entries
# holding a file name, its record count and its first record. Every record
# starts with a 6 byte header: compression type (0 = none), uncompressed
# size and stored size, big endian words.

CB_RECORD_MAX = 32000


def read_pdb(path):
    with open(path, 'rb') as f:
        b = f.read()
    hdr = b[:78]
    nrec = struct.unpack('>H', b[76:78])[0]
    offs = [struct.unpack('>I', b[78 + 8 * i:82 + 8 * i])[0]
            for i in range(nrec)]
    ends = offs[1:] + [len(b)]
    recs = [b[o:e] for o, e in zip(offs, ends)]
    return hdr, recs


def parse_dir(rec0):
    data = rec0[6:]
    entries = []
    for i in range(0, len(data), 32):
        e = data[i:i + 32]
        name = e[:29].split(b'\0')[0].decode('latin-1')
        crec = e[29]
        irec = struct.unpack('>H', e[30:32])[0]
        entries.append((name, crec, irec))
    return entries


def pack_uncompressed(data):
    out = []
    for i in range(0, max(len(data), 1), CB_RECORD_MAX):
        chunk = data[i:i + CB_RECORD_MAX]
        out.append(struct.pack('>HHH', 0, len(chunk), len(chunk)) + chunk)
    return out


def write_pdb(path, hdr, files):
    # The game finds files with a binary search, so keep strcmp order
    names = sorted(files, key=lambda s: [ord(c) for c in s])
    recs = []
    direntries = b''
    irec = 1
    for name in names:
        r = files[name]
        direntries += (name.encode('latin-1').ljust(29, b'\0') +
                bytes([len(r)]) + struct.pack('>H', irec))
        irec += len(r)
        recs += r
    recs.insert(0, struct.pack('>HHH', 0, len(direntries), len(direntries)) +
            direntries)
    nrec = len(recs)
    out = bytearray(hdr[:76]) + struct.pack('>H', nrec)
    off = 78 + 8 * nrec
    table = b''
    for i, r in enumerate(recs):
        uid = i + 1
        table += struct.pack('>I', off) + bytes([0]) + bytes(
                [uid & 0xff, (uid >> 8) & 0xff, (uid >> 16) & 0xff])
        off += len(r)
    out += table + b''.join(recs)
    tmp = path + '.tmp'
    with open(tmp, 'wb') as f:
        f.write(out)
    os.replace(tmp, path)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--inicrunch', help='path to inicrunch.exe')
    parser.add_argument('--pdb', default=os.path.join(build.GAME_DIR,
            build.PDB), help='the .pdb to update (default: game\\%s)' %
            build.PDB)
    args = parser.parse_args()

    inicrunch = build.find_inicrunch(args.inicrunch)
    pdb = os.path.abspath(args.pdb)
    os.chdir(build.DATA_DIR)

    replacements = {}
    with tempfile.TemporaryDirectory() as tmpdir:
        for src in build.files('*.ini.pp'):
            name = src[:-3]
            dst = os.path.join(tmpdir, name)
            build.log('Compiling %s...' % name)
            pp = build.Preprocessor([build.GAME_DIR])
            build.crunch_ini(pp, inicrunch, src, dst)
            with open(dst, 'rb') as f:
                replacements[name] = f.read()
    with open('help.txt', 'rb') as f:
        replacements['help.txt'] = f.read()
    for src in build.files('fonts/*.ttf'):
        with open(src, 'rb') as f:
            replacements[os.path.basename(src)] = f.read()
    replacements['version.txt'] = (build.DATA_VERSION + '\n').encode()

    # The game looks files up by lower case name
    hdr, recs = read_pdb(pdb)
    files = {}
    for name, crec, irec in parse_dir(recs[0]):
        files[name.lower()] = recs[irec:irec + crec]
    for name, data in replacements.items():
        files[name.lower()] = pack_uncompressed(data)
    write_pdb(pdb, hdr, files)
    build.log('Updated %s (%s)' % (pdb, ', '.join(sorted(replacements))))


if __name__ == '__main__':
    main()
