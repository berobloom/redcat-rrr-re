# RedCat's Razende Rekenrace Reverse Engineering

## Project goal

Reverse engineer the 1996 DOS game RedCat's Razende Rekenrace (Davilex) and
reimplement it in C, for the desktop (SDL2) and the browser (WebAssembly).

## Evidence rules

Never present an inference as fact.

Use these confidence levels:

- CONFIRMED
- STRONGLY SUPPORTED
- LIKELY
- SPECULATIVE

Prefer evidence from binaries and runtime experiments over external documentation.

## Important files

- `research/discoveries.md` — confirmed discoveries
- `research/hypotheses.md` — current hypotheses
- `research/open-questions.md` — unresolved questions
- `reverse-engineering/` — technical documentation
- `reverse-engineering/analysis/<prog>_names.txt` — function names per executable

## Workflow

Before investigating a subsystem:

1. Search existing research.
2. Check open questions.
3. Reuse existing discoveries.
4. Only then perform new analysis.

When discovering something important, update the appropriate research file.

Do not duplicate existing findings.

## Reverse engineering

Preserve original binaries.

Record executable addresses/offsets when possible. Addresses are linear
addresses as `reverse-engineering/tools/leparse.py` loads the LE image
(code object at 0x10000), which is also what DOS/4GW uses at run time.

Record evidence for important conclusions.

Prefer small experiments that distinguish competing hypotheses.

## Setup

```sh
7z x -o.work/iso "Redcat Razende Rekenrace.iso"
python3 -m venv .work/venv && .work/venv/bin/pip install capstone unicorn pillow
for p in RRR REKEN RRRMENU INTRO; do
  python3 reverse-engineering/tools/leparse.py .work/iso/$p.EXE .work/le/${p,,}
done
```
