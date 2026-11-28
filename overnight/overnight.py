#!/usr/bin/env python3
"""Self-healing overnight supervisor for pi (a Ralph-style loop: fresh context per pass, all state in
files and git).

Each pass: clean up the project's own harness processes, wait for the model server, gate on overnight/check.sh, run
one pi iteration with the watchdog extension, then judge the result:
  - server down / stall / timeout / crash  -> resume the same pi session (bounded), else move on
  - commits that fail the check            -> one in-session repair, then park them on an
                                              overnight-rejected/* branch and reset to the last good commit
  - leftover uncommitted changes           -> stashed (kept for review, tree made clean)
  - the same STATUS item failing           -> next prompt tells the agent to move it to BLOCKED
  - repeated timeouts / loops              -> step the thinking level down, back up after progress
The night ends on STOP file, deadline, pass limit, too many fruitless passes, or a dead server.
Everything is logged under overnight/runs/<stamp>/, with REPORT.md for the morning.

Usage:  overnight/run.sh [--hours H] [--passes N]      the night (inhibits sleep)
        overnight/overnight.py --smoke                 2-minute check that pi, model, tools and the
                                                       watchdog work; changes nothing
        overnight/overnight.py --dry-run               print the pi command and the first prompt
Stop gracefully: touch overnight/STOP (after the current pass). Stop now: kill <supervisor pid>.
"""
import argparse, fcntl, json, os, re, signal, subprocess, sys, threading, time, urllib.request, uuid
from datetime import datetime

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
CFG = json.load(open(os.path.join(HERE, 'config.json')))
WATCHDOG = os.path.join(HERE, 'watchdog.ts')
STOP = os.path.join(HERE, 'STOP')
LIST = CFG.get('list_name', 'HANDOFF')      # the STATUS.md list the agent takes items from

SERVER_ERR = re.compile(r'ECONNREFUSED|ECONNRESET|ETIMEDOUT|EHOSTUNREACH|fetch failed|socket hang up|'
                        r'connect(ion)? (refused|error|reset)|\b50[234]\b|terminated|network|Service Unavailable', re.I)
CONTEXT_ERR = re.compile(r'context (length|window|size)|too many tokens|exceeds? .*context|prompt is too long', re.I)

RESUME_PROMPT = {
    'server_down': 'The model server went away and is back. Continue the iteration exactly where you left off.',
    'stall': 'You were interrupted (no output for a long time). Continue where you left off; avoid the step that hung.',
    'crash': 'pi exited unexpectedly. Continue the iteration where you left off.',
    'aborted': 'Your run was interrupted. Continue the iteration where you left off.',
}
# A timed-out pass is not resumed: its context is near full, and the local model then spends the wrap-up
# compacting. A fresh session finishes the uncommitted work instead (see wrapup_prompt).
WRAP_AT = 0.75                    # fraction of the pass timeout at which the watchdog says "wrap up"


def sh(*cmd, check=False, timeout=None):
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=timeout)
    if check and r.returncode:
        raise RuntimeError(f'{" ".join(cmd)}: {r.stderr.strip() or r.stdout.strip()}')
    return r


def git(*a, check=True):
    return sh('git', *a, check=check).stdout.strip()


def head():
    return git('rev-parse', 'HEAD')


class Night:
    def __init__(self, args):
        self.args = args
        self.stamp = datetime.now().strftime('%Y%m%d-%H%M')
        self.dir = os.path.join(HERE, 'runs', self.stamp)
        self.sessions = os.path.join(self.dir, 'sessions')
        os.makedirs(self.sessions, exist_ok=True)
        self.deadline = time.time() + 3600 * (args.hours or CFG['max_hours'])
        self.max_passes = args.passes or CFG['max_passes']
        self.passes = []
        self.notes = []          # supervisor notes for the next prompt
        self.item_fails = {}
        self.fruitless = 0
        self.think = 0           # index into CFG['thinking']
        self.bad_streak = 0
        self.proc = None
        self.ended = ''
        self.foreign = set()

    # ---------- logging ----------
    def log(self, msg):
        line = f'[{datetime.now():%H:%M:%S}] {msg}'
        print(line, flush=True)
        with open(os.path.join(self.dir, 'supervisor.log'), 'a') as f:
            f.write(line + '\n')

    def journal(self, rec):
        with open(os.path.join(self.dir, 'journal.jsonl'), 'a') as f:
            f.write(json.dumps(rec) + '\n')

    # ---------- environment ----------
    def server_ok(self):
        try:
            with urllib.request.urlopen(CFG['server_health'], timeout=10) as r:
                body = json.loads(r.read() or b'{}')
            return r.status == 200 and body.get('loaded', True) is not False
        except Exception:
            return False

    def wait_server(self):
        if self.server_ok():
            return True
        self.log('model server down, waiting')
        t0, delay, restarted = time.time(), 15, False
        while time.time() - t0 < 60 * CFG['server_wait_max_min'] and time.time() < self.deadline:
            if self.stop_requested():
                return False
            if CFG.get('server_restart_cmd') and not restarted and time.time() - t0 > 300:
                self.log('running server_restart_cmd')
                subprocess.run(CFG['server_restart_cmd'], shell=True, cwd=ROOT)
                restarted = True
            time.sleep(delay)
            delay = min(delay * 2, 120)
            if self.server_ok():
                self.log(f'model server back after {int(time.time() - t0)}s')
                return True
        return False

    def owned_procs(self):
        """Harness processes this project started: cmdline matches `owned_processes` and the working
        directory is inside this repo. Not the command line: an interactive session or another project may launch it
        from a binary under this repo (the sibling projects run this same suite's dosbox-x), but it runs in its own directory."""
        pats = [re.compile(p) for p in CFG.get('owned_processes', [])]
        for p in os.listdir('/proc'):
            if not p.isdigit() or int(p) == os.getpid():
                continue
            try:
                cmd = open(f'/proc/{p}/cmdline', 'rb').read().replace(b'\0', b' ').decode('utf-8', 'replace').strip()
                cwd = os.readlink(f'/proc/{p}/cwd')
            except OSError:
                continue
            if cmd and any(r.search(cmd) for r in pats) and (cwd == ROOT or cwd.startswith(ROOT + '/')):
                yield int(p), cmd

    def cleanup(self):
        if CFG.get('cleanup_cmd'):
            out = sh(CFG['cleanup_cmd']).stdout.strip()
            if out:
                self.log(out)
        for sig in (signal.SIGTERM, signal.SIGKILL):
            victims = [(p, c) for p, c in self.owned_procs() if p not in self.foreign]
            for p, c in victims:
                try:
                    os.kill(p, sig)
                    self.log(f'cleanup: {sig.name} {p} {c[:80]}')
                except ProcessLookupError:
                    pass
            if not victims:
                break
            time.sleep(2)

    def check(self, rev='HEAD', base=None):
        r = sh(CFG['check_cmd'], rev, *([base] if base else []), timeout=900)
        return r.returncode == 0, (r.stdout + r.stderr).strip()[-3000:]

    def check_pass(self, pre, new):
        """Health check plus: the pass's commits must not touch protected paths. The watchdog only guards the
        edit/write tools; a `git add` of a protected file goes through bash."""
        ok, out = self.check(new, pre)   # guards compare against the pass's start
        prot = CFG['policy'].get('protectedPaths', [])
        hit = [f for f in git('diff', '--name-only', pre, new).splitlines()
               if any(f == p or f.startswith(p.rstrip('/') + '/') for p in prot)]
        if hit:
            ok, out = False, out + '\nFAIL: commits change protected paths (never modify these): ' + ', '.join(hit)
        return ok, out

    def stop_requested(self):
        return os.path.exists(STOP)

    def bail(self, msg):
        # preflight aborts must be visible in the run dir, not only on the launching terminal
        self.log('PREFLIGHT FAIL: ' + msg)
        sys.exit(msg)

    def first_item(self):
        try:
            for line in open(os.path.join(ROOT, 'STATUS.md')):
                m = re.match(r'\s*1\.\s+(.*)', line)
                if m:
                    return re.sub(r'[*`]', '', m.group(1))[:90]
        except OSError:
            pass
        return '?'

    def preserve(self, kind, rev):
        name = f'{CFG["branch_prefix"]}-{kind}/{self.stamp}-p{len(self.passes) + 1}'
        git('branch', '-f', name, rev)
        return name

    def stash_leftovers(self):
        if not git('status', '--porcelain'):
            return ''
        git('stash', 'push', '-u', '-m', f'overnight {self.stamp} pass {len(self.passes) + 1} leftovers')
        return git('rev-parse', '--short', 'stash@{0}')

    # ---------- pi ----------
    def pi_cmd(self, prompt, session, new, thinking):
        cmd = [CFG['pi'], '-p', '--mode', 'json', '-a', '-ns', *CFG.get('pi_extra_args', []), '--model', CFG['model'],
               '--thinking', thinking, '--session-dir', self.sessions, '-e', WATCHDOG]
        cmd += ['--session-id', session] if new else ['--session', session]
        return cmd + [prompt]

    def run_pi(self, prompt, session, new, timeout_s, tag, policy_extra=None):
        n = len(self.passes) + 1
        base = os.path.join(self.dir, f'pass{n:02d}{tag}')
        policy = dict(CFG['policy'], projectRoot=ROOT, startHead=head(),
                      softDeadline=int(1000 * (time.time() + WRAP_AT * min(timeout_s, self.deadline - time.time()))))
        policy.update(policy_extra or {})
        json.dump(policy, open(base + '.policy.json', 'w'))
        env = dict(os.environ, OVERNIGHT_POLICY=base + '.policy.json', OVERNIGHT_STATE=base + '.state.json')
        thinking = CFG['thinking'][self.think]
        cmd = self.pi_cmd(prompt, session, new, thinking)
        self.log(f'pi start ({tag or "fresh"}, thinking={thinking}, timeout={timeout_s // 60}m, session={session[:8]})')

        info = {'turns': 0, 'tools': 0, 'errors': [], 'settled': False, 'compactions': 0, 'compaction_failed': 0, 'last': time.time(), 'updates': {}}
        out = open(base + '.jsonl', 'w')
        err = open(base + '.stderr', 'w')
        self.proc = subprocess.Popen(cmd, cwd=ROOT, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                     stderr=err, start_new_session=True)

        pend = {'kind': '', 'd': [], 'n': 0, 't': 0.0}

        def flush_delta():
            if pend['d']:
                out.write(json.dumps({'type': 'delta', 'kind': pend['kind'], 'd': ''.join(pend['d'])}) + '\n')
                out.flush()
            pend.update(kind='', d=[], n=0)

        def reader():
            for raw in self.proc.stdout:
                info['last'] = time.time()
                try:
                    ev = json.loads(raw)
                except ValueError:
                    continue
                t = ev.get('type')
                if t == 'message_update':
                    # keep only text/thinking deltas for overnight/watch.py, merged into one line per
                    # second or 4 KB (one line per token was ~45 bytes of JSON per token)
                    a = ev.get('assistantMessageEvent') or {}
                    if a.get('type') in ('text_delta', 'thinking_delta'):
                        kind, d = a['type'][:-6], a.get('delta', '')
                        if kind != pend['kind']:
                            flush_delta()
                            pend.update(kind=kind, t=info['last'])
                        pend['d'].append(d)
                        pend['n'] += len(d)
                        if pend['n'] >= 4096 or info['last'] - pend['t'] >= 1:
                            flush_delta()
                            pend['kind'] = kind
                            pend['t'] = info['last']
                    continue
                flush_delta()
                if t == 'tool_execution_update':
                    # partialResult holds the FULL accumulated output every time -> O(n^2) bytes
                    # for chatty tools. Keep a compact marker; the full result lands in tool_execution_end.
                    cid = ev.get('toolCallId', '')
                    info['updates'][cid] = info['updates'].get(cid, 0) + 1
                    out.write(json.dumps({'type': 'tool_execution_update', 'toolCallId': cid,
                                          'toolName': ev.get('toolName', ''), 'n': info['updates'][cid]}) + '\n')
                    out.flush()
                    continue
                # Drop the copies of tool output: tool_execution_end's content (pi already caps it at
                # 50 KB) and message_end are kept; structuredContent (up to 1 MB per call), turn_end's
                # toolResults/message and agent_end's whole transcript repeat them, and the full
                # session is in sessions/ anyway. watch.py reads none of the dropped fields.
                if t == 'tool_execution_end' and isinstance(ev.get('result'), dict):
                    ev['result'].pop('structuredContent', None)
                elif t == 'turn_end':
                    ev['toolResults'] = len(ev.get('toolResults') or [])
                    ev.pop('message', None)
                elif t == 'agent_end':
                    ev['messages'] = len(ev.get('messages') or [])
                out.write(json.dumps(ev) + '\n' if t in ('tool_execution_end', 'turn_end', 'agent_end')
                          else raw.decode('utf-8', 'replace'))
                out.flush()
                if t == 'turn_end':
                    info['turns'] += 1
                elif t == 'tool_execution_start':
                    info['tools'] += 1
                elif t == 'agent_settled':
                    info['settled'] = True
                elif t == 'compaction_start':
                    info['compactions'] += 1
                elif t == 'auto_retry_end' and not ev.get('success'):
                    info['errors'].append(str(ev.get('finalError', '')))
                elif t == 'compaction_end' and ev.get('errorMessage'):
                    info['compaction_failed'] += 1
                    info['errors'].append(str(ev['errorMessage']))
                elif t == 'message_end':
                    m = ev.get('message') or {}
                    if m.get('role') == 'assistant' and m.get('errorMessage'):
                        info['errors'].append(str(m['errorMessage']))
            flush_delta()
        th = threading.Thread(target=reader, daemon=True)
        th.start()

        t0, killed = time.time(), ''
        hard_end = min(t0 + timeout_s, self.deadline)
        while self.proc.poll() is None:
            time.sleep(5)
            now = time.time()
            if now > hard_end:
                killed = 'timeout' if now < self.deadline else 'deadline'
            elif now - info['last'] > 60 * CFG['stall_min']:
                killed = 'stall'
            elif max(info['updates'].values(), default=0) > 300:   # runaway print loop: stalls never trip
                killed = 'runaway'
            if killed:
                self.kill_pi()
                break
        th.join(timeout=10)
        out.close(); err.close()
        rc = self.proc.returncode
        self.proc = None

        state = {}
        try:
            state = json.load(open(base + '.state.json'))
        except (OSError, ValueError):
            pass
        errors = ' | '.join(info['errors'])[-600:]
        if killed:
            kind = killed
        elif state.get('endReason') in ('looping', 'turn_budget'):
            kind = state['endReason']
        elif CONTEXT_ERR.search(errors):
            kind = 'context'
        elif (errors and SERVER_ERR.search(errors)) or not self.server_ok():
            kind = 'server_down'
        elif rc != 0 or not info['settled']:
            kind = 'crash'
        elif 'aborted' in errors.lower() and head() == policy['startHead']:
            kind = 'aborted'             # interrupted mid-run (e.g. by a compaction) without a commit
        else:
            kind = 'done'
        res = {'kind': kind, 'rc': rc, 'minutes': round((time.time() - t0) / 60, 1), 'turns': info['turns'],
               'tools': info['tools'], 'compactions': info['compactions'], 'errors': errors, 'thinking': thinking,
               'watchdog': {k: state.get(k) for k in ('policyBlocks', 'repeatBlocks', 'nudges',
                                                       'errorSteers', 'bashTimeouts', 'endReason', 'blocked')}}
        self.log(f'pi end: {kind} rc={rc} {res["minutes"]}m turns={info["turns"]} tools={info["tools"]}'
                 + (f' errors: {errors[:200]}' if errors else ''))
        return res

    def kill_pi(self):
        if not self.proc:
            return
        try:
            os.killpg(self.proc.pid, signal.SIGTERM)
            self.proc.wait(timeout=15)
        except subprocess.TimeoutExpired:
            os.killpg(self.proc.pid, signal.SIGKILL)
            self.proc.wait()
        except ProcessLookupError:
            pass

    # ---------- prompt ----------
    def wrapup_prompt(self):
        rules = open(os.path.join(HERE, 'prompt.md')).read()
        rules = rules[rules.find('Hard rules:'):] if 'Hard rules:' in rules else ''
        return (
            'You are finishing ONE iteration of this reverse-engineering project, unattended. The previous '
            'session hit its time limit before committing; its uncommitted work is in the working tree. '
            'Do NOT start new analysis.\n\n'
            '1. Run `git status` and `git diff` and read STATUS.md to see what it was doing.\n'
            '2. Keep what is coherent and supported by the diff; revert the rest with `git checkout -- <file>`. '
            'If a tool changed, regenerate what it produces.\n'
            '3. Make sure the findings files say what was found (verified vs inferred).\n'
            f'4. Update STATUS.md in its usual shape: iteration number + 1, the {LIST} list with at least 3 small items.\n'
            '5. `git add` the changed files, `git commit -m "iter N: <summary>"`, run `overnight/check.sh`, '
            'and fix + commit again until it prints OK.\n6. Stop.\n\n' + rules)

    def prompt(self, mode, check_out):
        p = open(os.path.join(HERE, 'prompt.md')).read()
        extra = []
        if mode == 'repair':
            extra.append(f'The health check FAILS on the current commit, so this iteration is a REPAIR: do not '
                         f'take a {LIST} item. Fix the cause below, commit as "iter N: repair <what>", and stop.\n'
                         '```\n' + check_out + '\n```')
        extra += self.notes
        if extra:
            p += '\n## Supervisor notes for this iteration\n' + '\n'.join(f'- {e}' for e in extra) + '\n'
        return p

    # ---------- the night ----------
    def preflight(self, clean=True):
        # harness processes already running in this repo belong to an interactive session: never
        # kill them, and do not start a night that would fight over the same ports
        self.foreign = {p for p, _ in self.owned_procs()}
        if self.foreign and CFG.get('require_harness_free'):
            self.bail('harness processes of this project are already running (another session?):\n' +
                      '\n'.join(f'  {p} {c[:100]}' for p, c in self.owned_procs()) +
                      '\nclose that harness first; the night would fight over the same gdb/QMP ports')
        if not os.path.exists(CFG['pi']):
            self.bail(f'pi not found at {CFG["pi"]}')
        prov, _, mid = CFG['model'].partition('/')
        models = sh(CFG['pi'], '--list-models', timeout=60).stdout
        if not any(l.split()[:2] == [prov, mid] for l in models.splitlines()):
            self.bail(f'model {CFG["model"]} not in `pi --list-models`:\n{models}')
        if not self.server_ok():
            self.bail(f'model server not healthy: {CFG["server_health"]}')
        if clean and git('status', '--porcelain', '--untracked-files=no'):
            # leftovers of an interrupted pass/session must not poison the whole night
            s = self.stash_leftovers()
            self.notes.append(f'Uncommitted changes found at night start were stashed ({s}); fold anything '
                              'still wanted from that stash back in if it is coherent.')
            self.log(f'preflight: stashed leftover changes to {s}')

    def ensure_branch(self):
        cur = git('branch', '--show-current')
        self.base = 'main' if git('rev-parse', '--verify', '--quiet', 'main', check=False) else 'master'
        if cur.startswith(CFG['branch_prefix'] + '/'):
            self.log(f'continuing on branch {cur}')
            return
        name, i = f'{CFG["branch_prefix"]}/{self.stamp[:8]}', 2
        while git('rev-parse', '--verify', '--quiet', name, check=False):
            name, i = f'{CFG["branch_prefix"]}/{self.stamp[:8]}-{i}', i + 1
        git('switch', '-c', name)
        self.log(f'created branch {name} from {cur}')

    def run(self):
        self.preflight()
        self.ensure_branch()
        self.night_start = head()
        ok, out = self.check()
        self.last_good = self.night_start if ok else None
        self.log(f'baseline check: {"OK" if ok else "FAIL"}')
        repairs = 0
        while True:
            if self.stop_requested():
                self.ended = 'STOP file'; break
            if len(self.passes) >= self.max_passes:
                self.ended = 'pass limit'; break
            if self.deadline - time.time() < 20 * 60:
                self.ended = 'deadline'; break
            if self.fruitless >= CFG['max_fruitless_passes']:
                self.ended = f'{self.fruitless} passes in a row without progress'; break
            self.cleanup()
            if not self.wait_server():
                self.ended = 'model server unavailable'; break

            pre = head()
            ok, out = self.check(pre)
            if ok:
                self.last_good, repairs = pre, 0
            else:
                repairs += 1
                if repairs > CFG['repair_attempts']:
                    if not self.last_good:
                        self.ended = 'health check failing and no known-good commit'; break
                    saved = self.preserve('rejected', pre)
                    git('reset', '--hard', self.last_good)
                    self.log(f'repairs failed: parked {pre[:8]} on {saved}, reset to {self.last_good[:8]}')
                    self.notes.append(f'Earlier commits broke the health check and were rolled back (kept on '
                                      f'branch {saved}). Redo that work more carefully.')
                    repairs = 0
                    continue
            mode = 'work' if ok else 'repair'
            item = self.first_item() if ok else 'repair'
            rec = self.one_pass(mode, out, pre, item)
            self.passes.append(rec)
            self.journal(rec)
            self.report()
        self.cleanup()
        self.log(f'night ended: {self.ended}')
        self.report()

    def one_pass(self, mode, check_out, pre, item):
        started = datetime.now().strftime('%H:%M')
        session = str(uuid.uuid4())
        prompt = self.prompt(mode, check_out)
        self.notes = []
        runs = [self.run_pi(prompt, session, True, 60 * CFG['pass_timeout_min'], '')]
        resumes = 0
        while resumes < CFG['max_resumes'] and time.time() < self.deadline - 600:
            kind = runs[-1]['kind']
            self.cleanup()
            if kind == 'timeout':
                if head() != pre or not git('status', '--porcelain'):
                    break                # committed already (leftovers get stashed), or nothing to finish
                resumes += 1
                runs.append(self.run_pi(self.wrapup_prompt(), str(uuid.uuid4()), True, 60 * 20, f'w{resumes}'))
                break
            if kind not in RESUME_PROMPT or (kind == 'server_down' and not self.wait_server()):
                break
            resumes += 1
            runs.append(self.run_pi(RESUME_PROMPT[kind], session, False, 60 * CFG['pass_timeout_min'] // 2, f'r{resumes}'))

        stash = self.stash_leftovers()
        new = head()
        check_ok, check_out2, rejected = None, '', ''
        if new != pre:
            check_ok, check_out2 = self.check_pass(pre, new)
            if not check_ok and time.time() < self.deadline - 600:
                self.log('commits fail the health check: in-session repair')
                runs.append(self.run_pi(
                    'overnight/check.sh FAILS on your commits. Fix the cause (do not touch overnight/ or original/), '
                    'commit, and stop.\n```\n' + check_out2 + '\n```', session, False, 60 * 20, 'fix'))
                stash = self.stash_leftovers() or stash
                new = head()
                check_ok, check_out2 = self.check_pass(pre, new)
            if not check_ok:
                rejected = self.preserve('rejected', new)
                git('reset', '--hard', pre)
                self.log(f'rejected {new[:8]} (kept on {rejected}), back to {pre[:8]}')
                self.notes.append('Your previous iteration was rolled back because overnight/check.sh failed:\n'
                                  '```\n' + check_out2[-1200:] + '\n```\nAvoid that mistake.')
                new = pre
        progress = new != pre
        commits = git('log', '--format=%h %s', f'{pre}..{new}').splitlines() if progress else []

        if progress:
            self.fruitless, self.bad_streak = 0, 0
            self.item_fails.pop(item, None)
            if self.think:
                self.think -= 1
        else:
            self.fruitless += 1
            self.item_fails[item] = self.item_fails.get(item, 0) + 1
            if self.item_fails[item] >= CFG['max_stuck_item'] and mode == 'work':
                self.notes.append(f'{LIST} item "{item}" failed {self.item_fails[item]} iterations in a row. Move it '
                                  'under BLOCKED in STATUS.md with what was tried and why it failed, commit, then '
                                  'work on the next item.')
            if runs[0]['kind'] in ('timeout', 'looping', 'turn_budget', 'context', 'stall'):
                self.bad_streak += 1
                if self.bad_streak >= 2 and self.think < len(CFG['thinking']) - 1:
                    self.think += 1
                    self.bad_streak = 0
                    self.log(f'thinking level down to {CFG["thinking"][self.think]}')
        if not progress and all(r['kind'] == 'done' for r in runs) and not rejected:
            self.notes.append('The previous iteration ended without a commit. Commit verified progress, even a '
                              'small step, before stopping.')
        self.log(f'pass {len(self.passes) + 1}: {"progress" if progress else "no progress"} {commits}')
        return {'pass': len(self.passes) + 1, 'started': started, 'mode': mode, 'item': item, 'session': session,
                'runs': runs, 'commits': commits, 'check': check_ok, 'rejected': rejected, 'stash': stash,
                'progress': progress}

    def report(self):
        L = [f'# Overnight report {self.stamp}', '',
             f'Branch `{git("branch", "--show-current")}`, model `{CFG["model"]}`.' +
             (f' Ended: **{self.ended}**.' if self.ended else ' (running)'), '']
        commits = git('log', '--format=- `%h` %s', f'{self.night_start}..HEAD').splitlines() if hasattr(self, 'night_start') else []
        L += [f'## Commits kept ({len(commits)})', *(commits or ['none']), '']
        L += ['## Passes', '| # | start | mode | item | runs | result |', '|---|---|---|---|---|---|']
        for p in self.passes:
            runs = ', '.join(f'{r["kind"]} {r["minutes"]}m/{r["turns"]}t' for r in p['runs'])
            res = 'progress' if p['progress'] else 'no progress'
            if p['rejected']:
                res += f', rejected → `{p["rejected"]}`'
            if p['stash']:
                res += f', leftovers stashed `{p["stash"]}`'
            wd = p['runs'][0]['watchdog']
            if wd.get('policyBlocks') or wd.get('repeatBlocks') or wd.get('nudges'):
                res += f' (blocks {wd.get("policyBlocks") or 0}/{wd.get("repeatBlocks") or 0}, nudges {wd.get("nudges") or 0})'
            if wd.get('bashTimeouts'):
                res += f' (bash timeouts {wd["bashTimeouts"]})'
            L.append(f'| {p["pass"]} | {p["started"]} | {p["mode"]} | {p["item"].replace("|", "/")} | {runs} | {res} |')
        base = getattr(self, 'base', 'main')
        L += ['', '## Review', f'- `git log --stat {base}..HEAD`, then merge into {base} if good.',
              '- Rejected work: `git branch --list "overnight-rejected/*"`. Leftovers: `git stash list`.',
              f'- Logs: `overnight/runs/{self.stamp}/` (passNN.jsonl = pi events, .state.json = watchdog counters).', '']
        try:
            L += ['## STATUS.md now', '```', open(os.path.join(ROOT, 'STATUS.md')).read().strip(), '```']
        except OSError:
            pass
        open(os.path.join(self.dir, 'REPORT.md'), 'w').write('\n'.join(L) + '\n')

    def smoke(self):
        self.preflight(clean=False)
        session = str(uuid.uuid4())
        res = self.run_pi('Smoke test, do exactly this and nothing else:\n'
                          '1. Use the bash tool to run: git rev-parse --short HEAD\n'
                          '2. Use the bash tool to run: pkill -f smoke-test-nonexistent\n'
                          '3. Use the bash tool to run: sleep 60\n'
                          '4. Reply with one line: the output of step 1, whether step 2 was blocked, '
                          'and whether step 3 timed out.',
                          session, True, 300, 'smoke', {'maxNudges': 0, 'bashTimeout': 5})
        wd = res['watchdog']
        ok = (res['kind'] == 'done' and res['tools'] >= 1 and (wd.get('policyBlocks') or 0) >= 1
              and (wd.get('bashTimeouts') or 0) >= 1)
        print(json.dumps(res, indent=1))
        print('SMOKE', 'OK' if ok else 'FAIL', f'(logs in {self.dir})')
        return ok


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--hours', type=float)
    ap.add_argument('--passes', type=int)
    ap.add_argument('--smoke', action='store_true')
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()

    lock = open(os.path.join(HERE, '.lock'), 'w')
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        sys.exit('another overnight run is active')

    night = Night(args)
    if args.dry_run:
        print(' '.join(night.pi_cmd('<prompt>', 'SESSION', True, CFG['thinking'][0])))
        ok, out = night.check()
        print(night.prompt('work' if ok else 'repair', out))
        return
    if args.smoke:
        sys.exit(0 if night.smoke() else 1)

    def on_term(signum, frame):
        night.log(f'signal {signum}: stopping')
        night.kill_pi()
        night.cleanup()
        night.ended = f'signal {signum}'
        night.report()
        sys.exit(1)
    signal.signal(signal.SIGTERM, on_term)
    signal.signal(signal.SIGINT, on_term)
    if os.path.exists(STOP):
        os.remove(STOP)
    night.run()


if __name__ == '__main__':
    main()
