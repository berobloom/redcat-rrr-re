#!/usr/bin/env python3
"""Live view of an overnight run: model thinking and replies as they stream, tool calls and results,
watchdog interventions, and supervisor decisions. Read-only; safe to start and stop at any time.

Usage:  python3 overnight/watch.py                 follow the newest run (waits for one to start)
        python3 overnight/watch.py --no-thinking   hide thinking tokens
        python3 overnight/watch.py --lines 20      show more of each tool result (default 8)
        python3 overnight/watch.py --replay        print the newest run from the start, then follow
        python3 overnight/watch.py runs/<stamp>    a specific run
"""
import argparse, glob, json, os, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
C = dict(dim='\033[2m', b='\033[1m', red='\033[31m', grn='\033[32m', yel='\033[33m', blu='\033[34m',
         mag='\033[35m', cyn='\033[36m', off='\033[0m')
if not sys.stdout.isatty():
    C = {k: '' for k in C}


def text_of(content):
    if isinstance(content, str):
        return content
    return '\n'.join(c.get('text', '') for c in content or [] if c.get('type') == 'text')


def short(v, n=300):
    s = v if isinstance(v, str) else json.dumps(v, ensure_ascii=False)
    s = s.replace('\n', ' ⏎ ')
    return s if len(s) <= n else s[:n] + '…'


class Follower:
    """Tails a file from an offset, yielding complete lines."""
    def __init__(self, path, start_at_end):
        self.path, self.buf = path, b''
        self.pos = os.path.getsize(path) if start_at_end else 0

    def lines(self):
        try:
            with open(self.path, 'rb') as f:
                f.seek(self.pos)
                data = f.read()
        except OSError:
            return []
        self.pos += len(data)
        self.buf += data
        *done, self.buf = self.buf.split(b'\n')
        return [l.decode('utf-8', 'replace') for l in done]


class View:
    def __init__(self, args):
        self.a = args
        self.mode = None          # 'thinking' | 'text' while streaming
        self.turn = 0

    def out(self, s='', end='\n'):
        sys.stdout.write(s + end)
        sys.stdout.flush()

    def end_stream(self):
        if self.mode:
            self.out(C['off'])
            self.mode = None

    def supervisor(self, line):
        self.end_stream()
        self.out(f'{C["b"]}{C["blu"]}■ {line}{C["off"]}')

    def event(self, line, name):
        try:
            ev = json.loads(line)
        except ValueError:
            return
        t = ev.get('type')
        if t == 'delta':
            if ev['kind'] == 'thinking' and self.a.no_thinking:
                return
            if self.mode != ev['kind']:
                self.end_stream()
                self.mode = ev['kind']
                self.out(f'{C["dim"]}💭 ' if self.mode == 'thinking' else f'{C["off"]}💬 ', end='')
            self.out(ev['d'].replace('\n', '\n   '), end='')
            return
        self.end_stream()
        if t == 'session':
            self.out(f'{C["b"]}── {name} ──{C["off"]}')
        elif t == 'turn_start':
            self.turn += 1
            self.out(f'{C["dim"]}── turn {self.turn} ──{C["off"]}')
        elif t == 'tool_execution_start':
            args = ev.get('args') or {}
            arg = args.get('command') or args.get('path') or args.get('file_path') or args
            self.out(f'{C["cyn"]}▶ {ev.get("toolName")}{C["off"]} {short(arg, 400)}')
        elif t == 'tool_execution_end':
            res = ev.get('result') or {}
            body = text_of(res.get('content')).rstrip().splitlines()
            col = C['red'] if ev.get('isError') else C['dim']
            ms = ev.get('durationMs')
            head = f'{col}  ◀ {"ERROR " if ev.get("isError") else ""}{len(body)} lines'
            self.out(head + (f', {ms / 1000:.1f}s' if ms else '') + C['off'])
            for l in body[:self.a.lines]:
                self.out(f'{col}  │ {l[:200]}{C["off"]}')
            if len(body) > self.a.lines:
                self.out(f'{col}  │ … {len(body) - self.a.lines} more{C["off"]}')
        elif t == 'message_start':
            m = ev.get('message') or {}
            if m.get('role') == 'custom':
                self.out(f'{C["yel"]}⚑ {m.get("customType")}: {text_of(m.get("content"))}{C["off"]}')
            elif m.get('role') == 'user':
                self.out(f'{C["grn"]}▷ prompt: {short(text_of(m.get("content")), 300)}{C["off"]}')
        elif t == 'message_end':
            m = ev.get('message') or {}
            if m.get('role') == 'assistant' and m.get('errorMessage'):
                self.out(f'{C["red"]}✖ model error: {m["errorMessage"]}{C["off"]}')
            u = m.get('usage') or {}
            if m.get('role') == 'assistant' and u.get('totalTokens'):
                self.out(f'{C["dim"]}  [{u.get("input", 0)} in / {u.get("output", 0)} out tokens, '
                         f'stop: {m.get("stopReason")}]{C["off"]}')
        elif t in ('compaction_start', 'compaction_end', 'auto_retry_start', 'auto_retry_end'):
            info = {k: v for k, v in ev.items() if k not in ('type', 'result')}
            self.out(f'{C["mag"]}↻ {t} {short(info, 200)}{C["off"]}')
        elif t == 'agent_settled':
            self.out(f'{C["dim"]}── settled{" (aborted)" if ev.get("aborted") else ""} ──{C["off"]}')


def newest_run():
    runs = sorted(glob.glob(os.path.join(HERE, 'runs', '*/')))
    return runs[-1] if runs else None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('run', nargs='?')
    ap.add_argument('--no-thinking', action='store_true')
    ap.add_argument('--lines', type=int, default=8)
    ap.add_argument('--replay', action='store_true')
    a = ap.parse_args()

    run = a.run and os.path.abspath(a.run)
    while not run:
        run = newest_run()
        if not run:
            print('waiting for a run to start (overnight/run.sh)…', end='\r', flush=True)
            time.sleep(2)
    view, followers, pinned = View(a), {}, bool(a.run)
    print(f'{C["b"]}watching {run}{C["off"]}  (Ctrl-C to quit; the run is not affected)')
    first = True
    try:
        while True:
            if not pinned and newest_run() != run:          # a new night started
                run, followers, first = newest_run(), {}, False
                view.supervisor(f'new run {run}')
            paths = [os.path.join(run, 'supervisor.log')] + sorted(glob.glob(os.path.join(run, 'pass*.jsonl')))
            for p in paths:
                if p not in followers and os.path.exists(p):
                    # existing files: from the end unless --replay; files created later: from the start
                    followers[p] = Follower(p, start_at_end=first and not a.replay and p.endswith('.log'))
                    if first and not a.replay and p.endswith('.jsonl') and p != paths[-1]:
                        followers[p].pos = os.path.getsize(p)
            first = False
            for p, f in followers.items():
                for line in f.lines():
                    if p.endswith('.log'):
                        view.supervisor(line)
                    else:
                        view.event(line, os.path.basename(p))
            time.sleep(0.2)
    except KeyboardInterrupt:
        view.end_stream()


if __name__ == '__main__':
    main()
