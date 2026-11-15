#!/usr/bin/env python3
"""Builds the game data file, game/htdata832.pdb, from the art, levels and
.ini files in this directory.

Usage (from any directory):

    python data\\build.py [--inicrunch PATH] [--clean]

Requirements:
  - Python 3.8 or later
  - The content tools in ..\\bin (bcr2.exe, acrunch.exe, mcl.exe, ...). They
    are .NET Framework programs; Windows asks to turn on .NET Framework 3.5
    the first time one runs if it isn't on already.
  - inicrunch.exe, built by the CMake build (it is found automatically in
    ..\\build\\bin\\<config>, or pass --inicrunch).

The data is built for 8 bit color and 32 pixel tiles (art from art824,
scaled), the only format the Windows game uses. Output goes to data\\832 and
the finished .pdb is copied to game\\htdata832.pdb, stamped with the game's
data version.
"""

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys

DATA_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(DATA_DIR)
TOOL_DIR = os.path.join(ROOT_DIR, 'bin')
GAME_DIR = os.path.join(ROOT_DIR, 'game')

# Data version stamped into version.txt. Must match gszVersion in
# game/game.cpp for release builds of the game.
DATA_VERSION = '1.6'

DEPTH = 8
TILESIZE = 32
DT = '%d%d' % (DEPTH, TILESIZE)
OUTDIR = DT
ARTDIR = 'art824'           # 32 pixel data is built from the 24 pixel art
PALDIR = ARTDIR
PDB = 'htdata%s.pdb' % DT
CREATORID = 'WARI'

PAL_SIZE = 256
FIXED_PAL_SIZE = 131
BACKGROUND_PAL_SIZE = 32

AMX_SCALEARG = ['-scale', '1.3333333333333', '-noscaleicon']
MAPBMP_SCALEARG = ['-scale', '1.3333333333333']
FORMBMP_SCALEARG = ['-scale', '1.0']
RAWBMP_SCALEARG = ['-scale', '1.0']

COMMON_PAL = '%s/common_%dbpp.pal' % (ARTDIR, DEPTH)
SHELL_PAL = '%s/shell_%dbpp.pal' % (ARTDIR, DEPTH)
FIXED_PAL = '%s/fixed_%dbpp.pal' % (PALDIR, DEPTH)
DESERT_PAL = '%s/desert_%dbpp.pal' % (PALDIR, DEPTH)
GRASSY_PAL = '%s/grassy_%dbpp.pal' % (PALDIR, DEPTH)

FONT_BMPS = ['%s/%s.bmp' % (ARTDIR, name) for name in
        ('standardfont', 'shadowfont', 'buttonfont', 'titlefont', 'hudfont')]

# Bitmaps drawn on the map; these are scaled up to the 32 pixel tile size
FIXED_MAP_BMP_NAMES = '''
    arrow0 arrow1 arrow2 arrow3 arrow4 arrow5 arrow6 arrow7
    fog0001 fog0010 fog0011 fog0100 fog0101 fog0111 fog1000 fog1010
    fog1011 fog1100 fog1101 fog1110 fog1111
    galax1a galax1b galax1c galax2a galax2b galax2c galax3a galax3b galax3c
    placementbad placementgood plant plant1 plant2 plant3 plant4 plant5
    rocketartifact rocks tree tree1 tree2 tree3 tree4 tree5 tree6 tree7
    scorch_8x8 scorch_16x16 scorch_32x16 scorch_48x48
    arrowheadleft arrowheadup arrowheadright arrowheaddown
    build_btn_up build_btn_down build_btn_disabled
    abort_build_btn_up abort_build_btn_down
    repair_btn_up repair_btn_down repair_btn_disabled
    abort_repair_btn_up abort_repair_btn_down
    sell_btn_up sell_btn_down sell_btn_disabled
'''.split()

# Missions and multiplayer maps, by terrain
GRASSY_LEVELS = '''S_00 S_01 S_02 S_03 S_04 S_05 S_06 C_02 C_03
    M_05 M_11 M_13 M_14 M_16 M_17 M_19 M_22'''.split()
DESERT_LEVELS = '''S_07 S_08 S_09 S_10 S_11 S_12 S_13 S_14
    C_01 C_04 C_05
    M_02 M_03 M_04 M_06 M_07 M_08 M_09 M_10 M_12 M_15 M_18 M_20
    M_21'''.split()


def log(msg):
    print(msg, flush=True)


def run(args):
    """Runs a tool from ..\\bin (or an absolute path)."""
    exe = args[0]
    if not os.path.isabs(exe):
        exe = os.path.join(TOOL_DIR, exe)
    cmd = [exe] + [str(a) for a in args[1:]]
    runner = os.environ.get('HT_TOOL_RUNNER')
    if runner:
        cmd = runner.split() + cmd
    result = subprocess.run(cmd, cwd=DATA_DIR)
    if result.returncode != 0:
        sys.exit('error: %s failed (exit code %d)' %
                (os.path.basename(exe), result.returncode))


def files(pattern):
    return sorted(os.path.relpath(p, DATA_DIR).replace('\\', '/')
            for p in glob.glob(os.path.join(DATA_DIR, pattern)))


#
# A minimal C preprocessor, sufficient for the .ini.pp files and levels:
# #include, object-like #define, #ifndef/#ifdef/#else/#endif, comments.
#

class Preprocessor:
    IDENT = re.compile(r'[A-Za-z_][A-Za-z0-9_]*')

    def __init__(self, include_dirs):
        self.include_dirs = include_dirs
        self.macros = {}

    def define_file(self, path):
        self.process_file(path, emit=False)

    def process_file(self, path, emit=True):
        with open(path, 'r', encoding='latin-1') as f:
            text = f.read()
        return self.process(text, os.path.dirname(path), emit)

    def process(self, text, cur_dir, emit=True):
        out = []
        stack = []  # active flags for nested conditionals
        in_comment = False
        for line in text.splitlines():
            line, in_comment = self.strip_comments(line, in_comment)
            stripped = line.strip()
            active = all(stack)
            if stripped.startswith('#'):
                self.directive(stripped[1:].strip(), stack, active, cur_dir)
                if emit:
                    out.append('')
                continue
            if emit:
                out.append(self.expand(line) if active else '')
        return '\n'.join(out) + '\n'

    def strip_comments(self, line, in_comment):
        result = []
        i = 0
        quote = None
        while i < len(line):
            if in_comment:
                end = line.find('*/', i)
                if end < 0:
                    return ''.join(result), True
                in_comment = False
                result.append(' ')
                i = end + 2
                continue
            c = line[i]
            if quote:
                result.append(c)
                if c == '\\' and i + 1 < len(line):
                    result.append(line[i + 1])
                    i += 2
                    continue
                if c == quote:
                    quote = None
                i += 1
                continue
            if c == '"':
                quote = c
            elif c == "'":
                # An unterminated ' runs to the end of the line, as in cpp
                if line.find("'", i + 1) >= 0:
                    quote = c
                else:
                    result.append(line[i:])
                    break
            elif line.startswith('//', i):
                break
            elif line.startswith('/*', i):
                in_comment = True
                i += 2
                continue
            result.append(c)
            i += 1
        return ''.join(result), in_comment

    def directive(self, d, stack, active, cur_dir):
        m = re.match(r'(\w+)\s*(.*)', d)
        if not m:
            return
        name, rest = m.group(1), m.group(2).strip()
        if name == 'ifdef':
            stack.append(rest.split()[0] in self.macros)
        elif name == 'ifndef':
            stack.append(rest.split()[0] not in self.macros)
        elif name == 'else':
            stack[-1] = not stack[-1]
        elif name == 'endif':
            stack.pop()
        elif not active:
            return
        elif name == 'define':
            m = re.match(r'([A-Za-z_][A-Za-z0-9_]*)(.*)', rest)
            self.macros[m.group(1)] = m.group(2).strip()
        elif name == 'undef':
            self.macros.pop(rest.split()[0], None)
        elif name == 'include':
            inc = rest[1:-1]
            for d in [cur_dir] + self.include_dirs:
                path = os.path.join(d, inc)
                if os.path.exists(path):
                    self.define_file(path)
                    break
            else:
                sys.exit('error: can\'t find include file ' + inc)
        else:
            sys.exit('error: unsupported directive #' + name)

    def expand(self, line, depth=0):
        result = []
        i = 0
        quote = None
        while i < len(line):
            c = line[i]
            if quote:
                result.append(c)
                if c == quote:
                    quote = None
                i += 1
                continue
            if c in '"\'':
                if c == "'" and line.find("'", i + 1) < 0:
                    result.append(line[i:])
                    break
                quote = c
                result.append(c)
                i += 1
                continue
            m = self.IDENT.match(line, i)
            if m and (i == 0 or not (line[i - 1].isalnum() or
                    line[i - 1] == '_')):
                word = m.group(0)
                if word in self.macros and depth < 32:
                    result.append(self.expand(self.macros[word], depth + 1))
                else:
                    result.append(word)
                i = m.end()
                continue
            result.append(c)
            i += 1
        return ''.join(result)


def find_inicrunch(path):
    if path:
        return os.path.abspath(path)
    candidates = []
    for config in ('Release', 'RelWithDebInfo', 'Debug', 'MinSizeRel', ''):
        candidates += glob.glob(os.path.join(ROOT_DIR, 'build*', 'bin',
                config, 'inicrunch.exe'))
    if not candidates:
        sys.exit('error: inicrunch.exe not found. Build it with CMake '
                'first, or pass --inicrunch PATH.')
    return max(candidates, key=os.path.getmtime)


def crunch_ini(pp, inicrunch, src, dst, extra_macros=None):
    """Preprocesses src and compiles it with inicrunch into dst."""
    if extra_macros:
        for f in extra_macros:
            pp.define_file(f)
    text = pp.process_file(os.path.join(DATA_DIR, src))
    tmp = os.path.join(DATA_DIR, 'ini.tmp')
    with open(tmp, 'w', encoding='latin-1', newline='\n') as f:
        f.write(text)
    run([inicrunch, 'ini.tmp', dst])
    os.remove(tmp)


def build(args):
    inicrunch = find_inicrunch(args.inicrunch)
    os.chdir(DATA_DIR)

    if args.clean and os.path.isdir(OUTDIR):
        shutil.rmtree(OUTDIR)
    os.makedirs(OUTDIR, exist_ok=True)

    # Palettes

    log('Building palettes...')
    amxs = files(ARTDIR + '/*.amx')
    pngs = []
    for amx in amxs:
        pngs += files(amx[:-4] + '/*.png')
    with open('files.tmp', 'w') as f:
        f.write(' '.join(pngs) + '\n')
    run(['paltool.exe', '-6', '-c', FIXED_PAL_SIZE, '-p', PAL_SIZE, '-s', '-t',
            '-o', FIXED_PAL, '-f', 'files.tmp', COMMON_PAL])
    os.remove('files.tmp')

    shell_raw_bmps = files(ARTDIR + '/shellrawbitmaps/*.bmp')
    run(['paltool.exe', '-6', '-c', PAL_SIZE, '-p', PAL_SIZE, '-s', '-t',
            '-o', SHELL_PAL, FIXED_PAL] + shell_raw_bmps)
    run(['palbin.exe', SHELL_PAL, OUTDIR + '/shell.palbin'])
    run(['shadowmap.exe', SHELL_PAL, OUTDIR + '/shell.palbin.shadowmap',
            '0.6'])

    for terrain, pal in (('desert', DESERT_PAL), ('grassy', GRASSY_PAL)):
        run(['mcl.exe', '-makepal', TILESIZE, terrain + '.tc', PAL_SIZE,
                FIXED_PAL_SIZE, BACKGROUND_PAL_SIZE, FIXED_PAL, pal])
        run(['palbin.exe', pal, '%s/%s.palbin' % (OUTDIR, terrain)])
        run(['shadowmap.exe', pal,
                '%s/%s.palbin.shadowmap' % (OUTDIR, terrain), '0.6'])

    # Animations

    log('Crunching animations...')
    for amx in amxs:
        log('  ' + amx)
        run(['acrunch.exe'] + AMX_SCALEARG + [FIXED_PAL, amx, OUTDIR])

    # Bitmaps

    log('Crunching bitmaps...')
    fixed_bmps = [b for b in files(ARTDIR + '/bitmaps/*.bmp')
            if b not in FONT_BMPS]
    map_bmps = ['%s/bitmaps/%s.bmp' % (ARTDIR, name)
            for name in FIXED_MAP_BMP_NAMES]
    form_bmps = [b for b in fixed_bmps if b not in map_bmps]
    run(['bcr2.exe'] + FORMBMP_SCALEARG + [FIXED_PAL, OUTDIR] + form_bmps)
    run(['bcr2.exe'] + MAPBMP_SCALEARG + [FIXED_PAL, OUTDIR] + map_bmps)
    run(['bcr2.exe', '-raw'] + RAWBMP_SCALEARG + [FIXED_PAL, OUTDIR] +
            files(ARTDIR + '/rawbitmaps/*.bmp'))
    run(['bcr2.exe', '-raw'] + RAWBMP_SCALEARG + [SHELL_PAL, OUTDIR] +
            shell_raw_bmps)
    run(['bcr2.exe', '-font', FIXED_PAL, OUTDIR] + FONT_BMPS)

    # Cutscene bitmaps each have their own palette

    for bmp in files(ARTDIR + '/cutscenebitmaps/*.bmp'):
        log('  cutscene ' + bmp)
        name = os.path.splitext(os.path.basename(bmp))[0]
        run(['paltool.exe', '-6', '-c', PAL_SIZE, '-p', PAL_SIZE, '-s', '-t',
                '-o', '_tmp.pal', COMMON_PAL, bmp])
        run(['bcr2.exe', '-raw'] + RAWBMP_SCALEARG + ['_tmp.pal', OUTDIR, bmp])
        run(['palbin.exe', '_tmp.pal', '%s/%s.palbin' % (OUTDIR, name)])
        os.remove('_tmp.pal')

    # Levels

    log('Compiling levels...')
    res_h = os.path.join(GAME_DIR, 'res.h')
    for levels, pal_args in (
            (DESERT_LEVELS, ['-1.0', '1.0', '1.0', '1.0', '1.0']),
            (GRASSY_LEVELS, ['0.8', '0.9', '0.9', '1.1', '1.3'])):
        run(['mcl.exe', '-levels', TILESIZE, DEPTH] + pal_args +
                [PALDIR, OUTDIR] + [l + '.ld' for l in levels])
        for level in levels:
            lvl = '%s/%s.lvl' % (OUTDIR, level)
            pp = Preprocessor([GAME_DIR])
            crunch_ini(pp, inicrunch, lvl, lvl, extra_macros=[res_h])

    # .ini files

    log('Compiling .ini files...')
    for src in files('*.ini.pp'):
        pp = Preprocessor([GAME_DIR])
        crunch_ini(pp, inicrunch, src, '%s/%s' % (OUTDIR, src[:-3]))

    # Help and strings

    shutil.copyfile('help.txt', OUTDIR + '/help.txt')
    run(['stringtable.exe', '../game/wistrings.h', OUTDIR + '/strings.bin'])

    # Pack it all up

    log('Packing %s...' % PDB)
    with open(OUTDIR + '/version.txt', 'w', newline='\n') as f:
        f.write(DATA_VERSION + '\n')
    pdb_tmp = PDB + '.tmp'
    if os.path.exists(pdb_tmp):
        os.remove(pdb_tmp)
    run(['packpdb2.exe', '-p', CREATORID, pdb_tmp, OUTDIR + '/*.*',
            '-nocompress', '*.snd', '*.tset*', '*.tbm', 'version.txt',
            'forms.ini'])
    shutil.move(pdb_tmp, os.path.join(GAME_DIR, PDB))
    log('Wrote game/%s' % PDB)


def main():
    parser = argparse.ArgumentParser(
            description='Builds the game data file game/htdata832.pdb.')
    parser.add_argument('--inicrunch', help='path to inicrunch.exe')
    parser.add_argument('--clean', action='store_true',
            help='delete previous build output first')
    build(parser.parse_args())


if __name__ == '__main__':
    main()
