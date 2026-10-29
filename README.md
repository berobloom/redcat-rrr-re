# RedCat's Razende Rekenrace: reverse engineering and port

A study of the 1996 DOS game **RedCat's Razende Rekenrace** (Davilex), a
racing game for primary-school arithmetic, and a reimplementation in C that
will run it on Linux and in the browser (WebAssembly).

The research half is notes on how the original works, worked out from its
executables (Watcom C, DOS/4GW) and its data. Each finding records the code
addresses it rests on and how sure it is.

The port is at an early stage: it reads all the game's archives and draws
the race maps and screens, on the desktop and in the browser. See
[`port/README.md`](port/README.md).

**The game itself is not included.** You need your own copy of the CD.
Nothing here changes or redistributes the original files.

## Layout

| Path | Contents |
|---|---|
| `research/discoveries.md` | What has been established, with evidence |
| `research/hypotheses.md` | Ideas that are not settled yet |
| `research/open-questions.md` | Open and resolved questions |
| `reverse-engineering/` | Reference docs: executables, file formats, architecture |
| `reverse-engineering/tools/` | LE loader, disassembler, function finder, xrefs |
| `reverse-engineering/analysis/` | Function lists, names, CD inventory |
| `port/` | The C port; [`port/README.md`](port/README.md) is its manual |
| `port/web/` | The WebAssembly build |
| `port/rctools/` | Python tools for the game's archives, images and maps |

## How the research works

Each finding carries a label: CONFIRMED, STRONGLY SUPPORTED, LIKELY or
SPECULATIVE. The notes never present an inference as fact. Evidence from the
binaries and from runtime experiments comes before outside documentation. The
working rules, and how to set up `.work/`, are in [`AGENTS.md`](AGENTS.md).

The 32-bit programs are LE executables, which most tools do not open.
`reverse-engineering/tools/leparse.py` applies their fixups and writes each
as an ELF image at its run-time addresses. The tools in
`reverse-engineering/tools/` need Python 3 with `capstone`.

## License

This software is released under the [BOLA License](LICENSE). It covers this
project's own code and notes, not the game.

## Legal

RedCat's Razende Rekenrace and its data are the property of their rights
holders (Davilex Software BV, 1996). This project contains no game data and
is meant for people who own the game.
