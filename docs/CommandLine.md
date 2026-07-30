# LittleBlitzer 3.01 command-line operation

LittleBlitzer 2.90 added optional unattended tournament operation. A batch run
loads the existing `Engines.lbe` and `Tournament.lbt` formats, starts without
button clicks, keeps the main window hidden, records progress, and exits when
the configured number of games has completed.

LittleBlitzer 2.91 adds a standard PGN `Termination` tag to every game. The
batch-queue supervisor validates those tags and reports the number of time
forfeits in its completion record.

LittleBlitzer 2.92 adds an atomic `<results>.manifest.json` provenance record
containing the executable, configuration, opening-file, and engine SHA-256
hashes; actual UCI identities; engine options; the opening seed; and tournament
settings. Engine process/protocol failures are now reported separately from
timeouts and illegal moves.

Launching `LittleBlitzer.exe` without `--batch` opens the normal graphical
interface. Version 2.93 refreshes that interface without changing batch-mode
arguments or behavior. Version 3.00 adds a persistent dark appearance option
without changing batch-mode arguments or behavior. Version 3.01 corrects
dark-mode text rendering in Tournament Settings, again without changing batch
operation.

## What changed in 2.90

- A tournament can be configured and started entirely from the command line.
- Engine, tournament, PGN, and status paths are explicit rather than selected
  through GUI controls.
- Batch runs close automatically and return a documented process exit code.
- A line-oriented status log reports startup, progress, completion, and setup
  errors without screen scraping.
- Existing PGNs are protected unless overwrite permission is explicit.
- An included PowerShell supervisor can preflight, run, and validate multiple
  tournaments sequentially.
- GUI operation and the established `.lbe`, `.lbt`, EPD, PGN, pairing, and
  adjudication behavior remain compatible.

## Running one tournament

```text
LittleBlitzer.exe --batch ^
  --engines "C:\tests\match\Engines.lbe" ^
  --settings "C:\tests\match\Tournament.lbt" ^
  --results "C:\tests\match\Results.pgn" ^
  --status "C:\tests\match\status.log"
```

The line continuations above use Command Prompt syntax. The command may also
be entered on one line.

| Argument | Required | Purpose |
| --- | --- | --- |
| `--batch` | Yes | Selects unattended operation. |
| `--engines <path>` | Yes | Supplies the existing LittleBlitzer engine configuration. |
| `--settings <path>` | Yes | Supplies the existing tournament configuration. |
| `--results <path>` | Yes | Selects the PGN output file. |
| `--status <path>` | No | Selects the status log. The default is `<results>.status.log`. |
| `--overwrite` | No | Permits truncating an existing results file before starting. |

Unknown arguments, missing values, and missing required arguments are treated
as setup errors.

## Completion and exit codes

Batch mode terminates the LittleBlitzer process after all active workers have
finished and the exact `Rounds` value from `Tournament.lbt` has been written.
The process exit code describes the outcome:

| Code | Meaning |
| ---: | --- |
| `0` | The requested number of games completed with no illegal-move or engine-process failure results. |
| `2` | Argument, input, engine-loading, output-file, or tournament-start failure. |
| `3` | The tournament completed, but at least one game ended because an engine produced an illegal move or suffered an initialization, process, or protocol failure. |

Because LittleBlitzer is a Windows graphical application, use a waiting
launcher when a script needs the exit code. For example, in PowerShell:

```powershell
$arguments = @(
    '--batch',
    '--engines', '"C:\tests\match\Engines.lbe"',
    '--settings', '"C:\tests\match\Tournament.lbt"',
    '--results', '"C:\tests\match\Results.pgn"',
    '--status', '"C:\tests\match\status.log"'
) -join ' '

$process = Start-Process `
    -FilePath 'C:\tools\LittleBlitzer.exe' `
    -ArgumentList $arguments `
    -Wait `
    -PassThru

$process.ExitCode
```

From Command Prompt, use `start /wait`:

```bat
start "" /wait LittleBlitzer.exe --batch --engines "C:\tests\match\Engines.lbe" --settings "C:\tests\match\Tournament.lbt" --results "C:\tests\match\Results.pgn" --status "C:\tests\match\status.log"
echo Exit code: %ERRORLEVEL%
```

## Status log

The status file is line-oriented and flushed after every update. It can be
read while the tournament is running. A successful run has records similar to:

```text
2026-07-27T01:14:39 START engines=C:\tests\match\Engines.lbe settings=C:\tests\match\Tournament.lbt results=C:\tests\match\Results.pgn
2026-07-27T01:14:39 RUNNING games=1024 parallel=24 engines=2
2026-07-27T01:14:40 PROGRESS completed=1 total=1024 active=24 illegal=0 engine_failures=0
2026-07-27T01:32:08 COMPLETE completed=1024 total=1024 illegal=0 engine_failures=0 exit=0
```

Setup failures add an `ERROR` record. A supervising program should check the
process exit code, the final `COMPLETE` record, and the completed game count in
the PGN rather than relying on only one of them.

## Files and paths

- Batch mode uses the same engine and tournament file formats as the GUI.
- Every started run creates `<results>.manifest.json`. Treat that manifest and
  its PGN as one result set.
- Quote paths containing spaces.
- Absolute `Engine=` paths are recommended.
- The directory containing `Tournament.lbt` becomes the working directory.
  Relative EPD and PGN starting-position paths are therefore resolved from
  that directory.
- Native batch mode requires the parent directories of the results and status
  paths to exist.
- Use a separate directory and fresh output names for each tournament.
- If `Results.pgn` already exists, the run fails safely unless `--overwrite`
  was supplied.
- Batch mode does not append to or resume a partial PGN. `--overwrite`
  deliberately starts a new result file.

Do not modify the engine list, tournament settings, opening file, engine
binaries, or companion network files while a run is active.

## Identity and reproducibility

`LB_Name` remains a display alias. LittleBlitzer 2.92 also preserves the
engine's real `id name` response and records both values in the run manifest.
An optional assertion can reject the wrong executable before the tournament:

```text
Engine=C:\engines\candidate.exe
LB_Name=Candidate build 32
LB_ExpectedUCI=Nullstar 032
Threads=1
```

`LB_ExpectedUCI` is LittleBlitzer metadata and is not sent as a UCI option.
Comparison is case-insensitive and otherwise exact.

`Tournament.lbt` may include a nonzero `OpeningSeed` value. A zero or omitted
value generates a new seed; the selected seed is always recorded in the
manifest as a decimal string, preserving the full 64-bit value. Reusing a
recorded seed with unchanged inputs reproduces the opening selection.

## Running a queue

The included
`scripts\Run-LittleBlitzerBatchQueue.ps1` supervisor runs multiple tournaments
sequentially. It performs a complete preflight before the first tournament,
then validates each completed run before advancing to the next one.

Example queue:

```json
{
  "tests": [
    {
      "name": "candidate versus baseline",
      "engines": "C:\\tests\\candidate-vs-baseline\\Engines.lbe",
      "settings": "C:\\tests\\candidate-vs-baseline\\Tournament.lbt",
      "results": "C:\\tests\\candidate-vs-baseline\\Results.pgn",
      "status": "C:\\tests\\candidate-vs-baseline\\status.log"
    },
    {
      "name": "candidate versus release",
      "engines": "C:\\tests\\candidate-vs-release\\Engines.lbe",
      "settings": "C:\\tests\\candidate-vs-release\\Tournament.lbt",
      "results": "C:\\tests\\candidate-vs-release\\Results.pgn",
      "status": "C:\\tests\\candidate-vs-release\\status.log"
    }
  ]
}
```

`status` is optional and defaults to `<results>.status.log`. `overwrite` is
also optional and defaults to `false`. Setting `"overwrite": true` is the
queue equivalent of supplying `--overwrite`.

Validate the complete queue without starting LittleBlitzer:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
    -File .\scripts\Run-LittleBlitzerBatchQueue.ps1 `
    -LittleBlitzer .\LittleBlitzer.exe `
    -Queue C:\tests\queue.json `
    -ValidateOnly
```

Run the queue:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
    -File .\scripts\Run-LittleBlitzerBatchQueue.ps1 `
    -LittleBlitzer .\LittleBlitzer.exe `
    -Queue C:\tests\queue.json
```

The optional `-Log` parameter selects the queue-level log. Its default is the
queue filename followed by `.run.log`.

Before starting, the supervisor verifies:

- the LittleBlitzer executable;
- every engine and tournament file;
- at least two `Engine=` entries and the referenced engine executables;
- the `Rounds` setting;
- any referenced EPD or PGN opening file;
- unique results paths; and
- overwrite authorization for any existing results.

After each tournament, it verifies:

- a zero LittleBlitzer exit code;
- exactly the configured number of completed PGN games;
- exactly one valid `Termination` tag per completed game;
- a matching successful `COMPLETE` status record;
- the absence of `illegal*` files in the results directory; and
- a readable run manifest whose recorded engine hashes still match; and
- the SHA-256 hash of the completed PGN.

If a tournament or validation fails, the supervisor stops and does not start
the remaining queue entries.

## Compatibility

Command-line operation is an additional front end to the normal tournament
implementation. It does not introduce a separate chess, pairing, opening, or
adjudication path. In particular, the colour-reversed EPD/PGN pairing behavior
introduced in LittleBlitzer 2.80 is retained in both GUI and batch operation.
