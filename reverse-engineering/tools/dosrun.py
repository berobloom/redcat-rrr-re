#!/usr/bin/env python3
"""Run the original under the dos-re-suite DOSBox-X and poke at it.

Needs reverse-engineering/dosbox/install.sh to have run (.work/dos/) and the
patched DOSBox-X from ../dos-re-suite ($DOS_RE_SUITE, $DOS_RE_DOSBOX_BINARY).
DOSBox-X runs on its own Xvfb display, so nothing pops up; screenshots come
from QMP.

usage: dosrun.py start                 launch (no-op if running)
       dosrun.py stop
       dosrun.py type 'RRR /DEBUG'     type a line at the DOS prompt (+Enter)
       dosrun.py key ret [secs]        tap (or hold) a QEMU qcode
       dosrun.py shot out.png          VGA screenshot
       dosrun.py mem 0xADDR len [out]  linear memory dump (hexdump or file)
       dosrun.py find hexbytes [start len]   search linear memory
       dosrun.py save name | load name save states in .work/dos/states/
       dosrun.py regs                  halt, print registers, resume
       dosrun.py poke 0xADDR hexbytes  halt, write linear memory, resume
       dosrun.py locate rrr            where DOS/4GW loaded rrr's objects

As a module: `with Dos() as d: d.type_line(...); d.mem(...)`.
"""
import os
import shutil
import socket
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SUITE = Path(os.environ.get('DOS_RE_SUITE', ROOT.parent / 'dos-re-suite'))
sys.path.insert(0, str(SUITE / 'lib'))
from dos_re_harness.remote_capture import QmpClient, RspClient  # noqa: E402

BINARY = Path(os.environ.get(
    'DOS_RE_DOSBOX_BINARY',
    SUITE / '.work' / 'dosbox-x-remotedebug' / 'src' / 'dosbox-x'))
DOS = ROOT / '.work' / 'dos'
CONF = DOS / 'redcat.conf'
STATES = DOS / 'states'
XDISPLAY = os.environ.get('RC_XDISPLAY', ':57')

QCODE = {' ': 'spc', '/': 'slash', '\\': 'backslash', '.': 'dot',
         ':': ('shift', 'semicolon'), '-': 'minus', '_': ('shift', 'minus'),
         '>': ('shift', 'dot'), '<': ('shift', 'comma'),
         '\n': 'ret'}


def _port_open(port):
    s = socket.socket()
    s.settimeout(0.3)
    try:
        s.connect(('127.0.0.1', port))
        return True
    except OSError:
        return False
    finally:
        s.close()


def running():
    return _port_open(4444)


def start(wait=10.0):
    if running():
        return 'already running'
    if not CONF.exists():
        raise SystemExit('run reverse-engineering/dosbox/install.sh first')
    env = dict(os.environ)
    if shutil.which('Xvfb') and not os.environ.get('RC_SHOW'):
        if not Path('/tmp/.X11-unix/X' + XDISPLAY[1:]).exists():
            subprocess.Popen(['Xvfb', XDISPLAY, '-screen', '0', '1024x768x24'],
                             stdout=subprocess.DEVNULL,
                             stderr=subprocess.DEVNULL, start_new_session=True)
            time.sleep(1)
        env['DISPLAY'] = XDISPLAY
        env.pop('WAYLAND_DISPLAY', None)
    env['SDL_AUDIODRIVER'] = os.environ.get('SDL_AUDIODRIVER', 'dummy')
    subprocess.Popen([str(BINARY), '-conf', str(CONF)], cwd=str(DOS), env=env,
                     stdout=open(DOS / 'dosbox.log', 'w'),
                     stderr=subprocess.STDOUT, start_new_session=True)
    t = time.time() + wait
    while time.time() < t:
        if running() and _port_open(2159):
            # the GDB stub holds the machine at boot until a client says
            # continue; release it and detach
            r = RspClient('127.0.0.1', 2159, 5.0)
            r.continue_nowait()
            time.sleep(0.3)
            r.close()
            time.sleep(2.0)            # let the autoexec finish
            return 'started'
        time.sleep(0.3)
    raise SystemExit('DOSBox-X did not open QMP; see .work/dos/dosbox.log')


def stop(wait=5.0):
    # SIGKILL: a SIGTERM makes DOSBox-X ask "quit anyway?" in a dialog that
    # inherits (and keeps open) the QMP and GDB ports
    subprocess.run(['pkill', '-9', '-f', str(CONF)], check=False)
    t = time.time() + wait
    while time.time() < t and (running() or _port_open(2159)):
        time.sleep(0.2)


class Dos:
    def __init__(self):
        self.q = QmpClient('127.0.0.1', 4444, 10.0)

    def __enter__(self):
        return self

    def __exit__(self, *a):
        self.q.close()

    def key(self, qcode, hold=0.08):
        if isinstance(qcode, tuple):
            self.q.key_chord(list(qcode), hold)
        else:
            self.q.key_tap(qcode, hold)
        time.sleep(0.03)

    def type_line(self, text, enter=True):
        for ch in text:
            if ch in QCODE:
                self.key(QCODE[ch])
            elif ch.isupper():
                self.key(('shift', ch.lower()))
            else:
                self.key(ch)
        if enter:
            self.key('ret')

    def shot(self, path):
        try:
            data = self.q.screendump()
        except RuntimeError:
            # DOSBox-X names captures after the running program
            # (rrr_001.png), which the QMP verb does not look for: take the
            # newest capture instead
            time.sleep(0.3)
            caps = sorted((DOS / 'capture').glob('*.png'),
                          key=lambda f: f.stat().st_mtime)
            if not caps:
                raise
            data = caps[-1].read_bytes()
        Path(path).write_bytes(data)

    def mem(self, addr, size):
        out = b''
        while size > 0:
            n = min(size, 0x10000)
            out += self.q.memdump(addr, n)
            addr += n
            size -= n
        return out

    def find(self, needle, start=0x100000, size=0xF00000):
        hits, chunk = [], 0x40000
        for a in range(start, start + size, chunk):
            b = self.mem(a, chunk + len(needle))
            p = b.find(needle)
            while p >= 0:
                if p < chunk:
                    hits.append(a + p)
                p = b.find(needle, p + 1)
        return hits

    def save(self, name):
        STATES.mkdir(parents=True, exist_ok=True)
        return self.q.save_state(STATES / (name + '.sav'))

    def load(self, name):
        return self.q.load_state(STATES / (name + '.sav'))


def locate(d, prog):
    """Where DOS/4GW loaded each object of a .work/le program: returns
    [(file_base, runtime_base)].  Searches memory for a 64-byte window that
    no fixup touches (raw LE bytes == fixed-up image bytes)."""
    sys.path.insert(0, str(Path(__file__).parent))
    from lelib import Image
    from leparse import LE
    img = Image(prog)
    exe = next((ROOT / '.work' / 'iso').glob(prog.upper() + '.EXE'), None) \
        or next((ROOT / '.work' / 'iso').glob(prog + '.exe'))
    le = LE(exe.read_bytes())
    out = []
    for o, (base, fixed, _) in zip(le.objects, img.objs):
        raw = le.object_image(o)[:o['page_count'] * le.page_size]  # no BSS
        n, votes = 64, {}
        # windows spread over the object; a delta must win most of them
        # (a single window can hit a stale copy of the file in memory)
        for k in range(1, 9):
            off = (len(raw) * k // 10) & ~15
            for off in range(off, min(off + 0x400, len(raw) - n), 16):
                w = fixed[off:off + n]
                if w == raw[off:off + n] and len(set(w)) > 12:
                    for h in d.find(bytes(w), 0, 0x1000000):
                        votes[h - off] = votes.get(h - off, 0) + 1
                    break
        best = max(votes, key=votes.get) if votes else None
        out.append((base, best))
    return out


def regs():
    r = RspClient('127.0.0.1', 2159, 10.0)
    try:
        r.halt(5.0)
        regs = r.registers()
        r.continue_nowait()
        return regs
    finally:
        r.close()


def poke(addr, data):
    r = RspClient('127.0.0.1', 2159, 10.0)
    try:
        r.halt(5.0)
        r.write_memory(addr, data)
        r.continue_nowait()
    finally:
        r.close()


def main(argv):
    cmd, args = argv[1], argv[2:]
    if cmd == 'start':
        print(start())
    elif cmd == 'stop':
        stop()
    elif cmd == 'poke':
        poke(int(args[0], 0), bytes.fromhex(args[1]))
    elif cmd == 'regs':
        for k, v in regs().items():
            print('%-6s %08x' % (k, v))
    else:
        with Dos() as d:
            if cmd == 'type':
                d.type_line(' '.join(args))
            elif cmd == 'key':
                d.key(args[0], float(args[1]) if len(args) > 1 else 0.08)
            elif cmd == 'shot':
                d.shot(args[0])
            elif cmd == 'mem':
                b = d.mem(int(args[0], 0), int(args[1], 0))
                if len(args) > 2:
                    Path(args[2]).write_bytes(b)
                else:
                    for i in range(0, len(b), 16):
                        print('%08x  %s' % (int(args[0], 0) + i,
                                            b[i:i + 16].hex(' ')))
            elif cmd == 'find':
                st = int(args[1], 0) if len(args) > 1 else 0x100000
                sz = int(args[2], 0) if len(args) > 2 else 0xF00000
                for h in d.find(bytes.fromhex(args[0]), st, sz):
                    print(hex(h))
            elif cmd == 'locate':
                for fb, rb in locate(d, args[0]):
                    print('object 0x%x -> %s (delta %s)' % (
                        fb, hex(rb) if rb is not None else '?',
                        hex(rb - fb) if rb is not None else '?'))
            elif cmd == 'save':
                print(d.save(args[0]))
            elif cmd == 'load':
                print(d.load(args[0]))
            else:
                print(__doc__)
                return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
