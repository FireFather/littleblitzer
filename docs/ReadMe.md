# LittleBlitzer

<div align="left">

  [![Release][release-badge]][release-link]
  [![Commits][commits-badge]][commits-link]

  ![Downloads][downloads-badge]
  [![License][license-badge]][license-link]
 
</div>

LittleBlitzer is a Windows tournament manager and testing application for UCI chess engines.

It can run up to 128 games concurrently for high-throughput engine testing. LittleBlitzer concurrency is
separate from each engine's own UCI thread setting.

Tournament results are written as PGN for rating tools such as Ordo, BayesElo, and EloStat.

![LittleBlitzer 2.93 light interface](https://raw.githubusercontent.com/FireFather/littleblitzer/master/bitmaps/LittleBlitzer_2.93.png)
![LittleBlitzer 3.00 dark interface](https://raw.githubusercontent.com/FireFather/littleblitzer/master/bitmaps/LittleBlitzer_3.00.png)

The live interface provides:

- A fixed Match summary with configuration, elapsed time, and estimated time remaining
- An aligned Results table with points/games, score, W-L-D, milliseconds per move, depth, and NPS
- Separate Losses and Draws tables with adjudication and termination-reason counts
- A Copy report button that copies the summary and complete live report as plain text
- A persistent Dark mode option for the main window and Tournament Settings dialog

### Illegal-move diagnostics

Enable **Write illegal moves to file** to create a uniquely named
`illegal_<engine>_<suffix>` text report whenever an engine returns an illegal move. Each report includes the
complete board dump and FEN, starting position, full move list, latest engine output, and a ready-to-paste UCI
`position fen ... moves ...` command for reproducing the failure. Reports are written to the tournament's
working directory.

---------
### LittleBlitzer 3.00
30/07/2026
- Adds a persistent Dark mode checkbox to Options
- Applies dark styling to controls, panels, inputs, results, scrollbars, and compatible window chrome
- Extends dark styling to the Tournament Settings dialog
- Retains the standard light appearance when Dark mode is disabled
- Keeps GUI configuration files, batch operation, tournament behavior, and output formats compatible

### LittleBlitzer 2.93
30/07/2026
- Refreshes the interface with native Windows visual styles, Segoe UI controls, and a Consolas results view
- Organizes Run control, Options, Match summary, and Live results into clear bordered panels
- Moves static match settings and elapsed/remaining time into the dedicated Match summary
- Uses a wider, equal-column grid for Results, Losses, and Draws, with adjudication counts clearly identified
- Labels LittleBlitzer parallelism as concurrent games to distinguish it from engine threads
- Keeps the main window at its designed width while retaining vertical resizing
- Copies the Match summary and complete Live results report through the Copy report button

### LittleBlitzer 2.92
30/07/2026
- Records the real UCI identity and SHA-256 hash of every engine in an atomic JSON run manifest
- Optional `LB_ExpectedUCI` engine setting rejects a mislabeled or unexpected executable before play
- Separately identifies engine process/protocol failures instead of reporting them as time forfeits or illegal moves
- Uses explicit inherited-handle lists and best-effort Windows Job Object containment for safer concurrent engine launches
- Stops safely if PGN output can no longer be written and flushed
- Adds reproducible opening seeds, stricter tournament/FEN validation, and corrected repetition and PGN edge cases
- Includes automated normal, illegal-move, timeout, process-death, manifest, and PGN regression tests
- Replaces shifting tab-separated GUI statistics with aligned Results and Terminations tables
- Uses each engine's UCI `id name` by default, with optional `LB_Name` aliases for self-play or custom labels

### LittleBlitzer 2.91
29/07/2026
- Standard PGN `Termination` tags now identify normal finishes, time forfeits, rules infractions, and adjudications
- Batch-queue validation now verifies every `Termination` tag and reports time-forfeit counts

### LittleBlitzer 2.90
27/07/2026
- Optional unattended batch mode with explicit engine, tournament, results, and status paths
- Sequential batch-queue supervisor with preflight and completed-PGN validation
- See [Command-line operation](CommandLine.md) for syntax, exit codes, safety behavior, and queue examples

### LittleBlitzer 2.80
23/07/2026
- Correctly shares each selected EPD/PGN opening between colour-reversed game pairs in both gauntlet and round-robin tournaments

### LittleBlitzer 2.77
03/01/2024
- Live win% for each engine calculated and displayed

### LittleBlitzer 2.76
19/12/2023
- 64 bit
- concurrency up to 128 threads
- Application window size increase
- Window is now horizontally and vertically resizable
- Includes Visual Studio 2022 project files
  
---------
for more info see LBG_hints.rtf, available at Stefan Pohl Computer Chess: https://www.sp-cc.de/

In Downloads: find the 'Download a short manual and some other helpful files for using the LittleBlitzerGUI here' link

-----------
LittleBlitzer is from http://www.kimiensoftware.com

Contact the author at nathanthom@gmail.com

### Historical interface: LittleBlitzer 2.77

For comparison and reference:

![LittleBlitzer 2.77 interface](https://raw.githubusercontent.com/FireFather/littleblitzer/master/bitmaps/LittleBlitzer_2.77.png)

[license-badge]: https://img.shields.io/github/license/FireFather/littleblitzer?style=for-the-badge&label=license&color=success
[license-link]: https://github.com/FireFather/littleblitzer/blob/master/docs/LICENSE
[release-badge]: https://img.shields.io/github/v/release/FireFather/littleblitzer?style=for-the-badge&label=official%20release
[release-link]: https://github.com/FireFather/littleblitzer/releases/latest
[commits-badge]: https://img.shields.io/github/commits-since/FireFather/littleblitzer/latest?style=for-the-badge
[commits-link]: https://github.com/FireFather/littleblitzer/commits/master
[downloads-badge]: https://img.shields.io/github/downloads/FireFather/littleblitzer/total?color=success&style=for-the-badge
