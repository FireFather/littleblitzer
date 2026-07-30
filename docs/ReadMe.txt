
LittleBlitzer is a Windows tournament manager for UCI chess engines. It was created to run thousands of
very fast games concurrently for engine testing. It creates PGN output suitable for rating tools such as
Ordo, BayesElo, and EloStat.

![LittleBlitzer 3.00 dark interface](https://raw.githubusercontent.com/FireFather/littleblitzer/master/bitmaps/LittleBlitzer_3.00.png)

Download LittleBlitzer from http://www.kimiensoftware.com
Contact me at nathanthom@gmail.com


## Instructions
- 1. Create an Engines.lbe file with a list of UCI engines to use. Do not put spaces either side of the =.
   Blank lines are fine, UCI parameters should appear after the relevant Engine line, so in the following
   example "Time Buffer" is only set for t20090922.exe and "Hash=128" is only set for Hamsters.exe. You can 
   LittleBlitzer uses the engine's UCI "id name" by default. Use the special variable LB_Name only when
   an explicit display alias is needed, such as distinguishing two self-play builds with the same UCI name.
   LB_ExpectedUCI may also be supplied to require an exact UCI "id name" value; a mismatch prevents
   the engine from being loaded.

		Engine=C:\Projects\LittleThought\Release\LittleThought 1.06.54.exe
		NumThreads=2
		TraceLevel=1
		LB_Name=LT 1.06.54
		LB_ExpectedUCI=LittleThought 1.06.54
		
		Engine=C:\Downloads\Engines\Twisted\twisted20090922\t20090922.exe
		Time Buffer=100
		
		Engine=C:\Downloads\Engines\AnMon\AnMon_5.75.exe
		
		Engine=C:\Downloads\Engines\Hamsters_0.0.6_Win32\Hamsters.exe
		Hash=128


- 2. Click the Load Engines button

- 3. Click the Load settings button. This will load settings from the specified Tournament.lbt file if it exists, otherwise uses defaults. Make whatever changes are required and OK to save.

Num Total Rounds:
   Total number of games to play, distributed across the configured concurrent games.

Num Parallel Tournaments:
   Number of games LittleBlitzer runs concurrently. For example, on a four-core system, start with no more
   than four concurrent games. Engine UCI thread settings multiply the total CPU demand.

Hash:
   Size of Hash tables for all engines (MB) (UCI parameter = Hash)

Ponder:
   TBD

Own Book:
   Tell each engine to use their own book if they have one (UCI parameter = OwnBook).

Type:
   Gauntlet: 1st engine vs all others repeatedly. 1st engine plays many more games than other engines.
   Round-Robin: 1vs2, 1vs3, 2vs1, 2vs3, 3vs1, 3vs2 etc. All engines play the same number of games.

Variant: 
   Standard
   Chess960 (FRC)

Adjudications:
   Mate Score/Mate Moves: If one engine thinks it is winning by at least this score (+ve, centi-pawns) and  the other engine thinks its losing by at least this score, and this situation has held for the specified number of moves, LittleBlitzer will terminate the game as a win/loss.
   Draw Moves: If a game goes for this many moves it is declared a draw, regardless of score/time.

Time Control:
   Fixed Time/Move: This time (in ms) will be used for every move - useful if the engines get lots of time losses on other time controls.
   Blitz: Uses a total base time and an increment which is added to the base time after making each move.
   Tournament: Specify a time in which the specified number of moves must be made. When the moves have been made, the clock/moves left are reset to the starting values, repeatedly.

Starting Positions:
   Opening: Always use a standard opening (or randomised opening for Chess960 variant).
   FEN: Specify a single position via a FEN string.
   EPD: Load starting positions from an EPD file. Positions are used sequentially and fairly - i.e. each engine plays both white and black against each opponent.
   PGN: Load starting positions from a PGN file. If it sees [FEN] tags it will load that position. If not,  it will play through all moves shown in the game and the resulting position will form the starting position.
   Randomize: Randomly selects from the loaded opening positions. Colour-reversed paired games use the same position so that each engine plays both White and Black from it.
   OpeningSeed: Optional nonzero seed for reproducible randomized opening selection. Generated seeds are
   recorded in the run manifest.


- 4. Click the Start button. If the results.pgn file specified already exists, you will be asked to append or overwrite.
   LittleBlitzer also writes Results.pgn.manifest.json with exact engine identities, SHA-256 hashes,
   settings, opening data, and the opening seed.

- 5. Use the Pause/Resume/+/- buttons to control the number of concurrent games while it is running. Try to minimise how often you do this, as it can affect the total number of games played. Specifically, the first engine will tend to get more games each time additional concurrent games are added.

- 6. When done, set concurrent games to zero and wait for active games to finish. Then run the Results.pgn file through Bayeselo, using commands something like:

   readpgn results1.pgn
   readpgn results2.pgn
   readpgn results3.pgn
   elo
   mm
   covariance
   ratings
   x


## Unattended batch mode
LittleBlitzer 2.90 can run an existing Engines.lbe and Tournament.lbt without GUI interaction:

   LittleBlitzer.exe --batch --engines "C:\path\Engines.lbe" --settings "C:\path\Tournament.lbt" --results "C:\path\Results.pgn" --status "C:\path\status.log" --overwrite

The --engines, --settings, and --results arguments are required. --status is optional and defaults to
Results.pgn.status.log. If the results file already exists, batch mode fails unless --overwrite is supplied.
The process exits with code 0 after the exact configured number of games, 2 for setup/start failures, or 3 if
one or more games ended because an engine produced an illegal move or failed to initialize during a game.
Relative Position paths in Tournament.lbt are resolved from the directory containing Tournament.lbt.

The included scripts\Run-LittleBlitzerBatchQueue.ps1 supervisor preflights and runs multiple batch-mode
tournaments sequentially, then checks the exit code, exact PGN game count, completion status, illegal-move
files, and PGN SHA-256. Use its -ValidateOnly switch to check a queue without starting any games.

See CommandLine.md for complete syntax, exit codes, safety behavior, and queue examples.



## Notes
- Enabling FRC via Variant=1 will automatically set each engine's UCI_Chess960 parameter to true.
- Parameters specified within the engines file will override any tournament settings, e.g. Hash size.
- For EPD opening suites, each position will get played 2*(numengines-1) times, so best to have number of Rounds a multiple of this but also evenly divisible by the number of parallel tournaments.
- For mate adjudication, both engines must agree on the score e.g. engine1 > 900 and engine2 < -900


## Live Results
The match configuration and elapsed/remaining time appear in the fixed Match summary panel. The scrolling
Live results pane contains the completed-game count followed by three aligned tables:

1. Results: points/games, score percentage, W-L-D, average move time in milliseconds (ms/move), depth, and NPS.
2. Losses: adjudication, mate, timeout, illegal move, and engine crash/death.
3. Draws: adjudication, repetition, insufficient material, 50-move rule, and stalemate.

The Copy report button copies the Match summary, time, and complete Live results report together as plain text.

Engine names longer than the table column are abbreviated there and printed in full beneath the tables.


## Illegal Move Reports
Enable Write illegal moves to file under Options to create a uniquely named illegal_<engine>_<suffix> text
report whenever an engine returns an illegal move. Each report includes the complete board dump and FEN,
starting position, full move list, latest engine output, and a ready-to-paste UCI
"position fen ... moves ..." command for reproducing the failure. Reports are written to the tournament's
working directory.


## Version History
- v3.00 30/7/2026
  Added a persistent Dark mode checkbox under Options.
  Applied dark styling to controls, panels, inputs, results, scrollbars, and compatible window chrome.
  Extended dark styling to the Tournament Settings dialog.
  Retained the standard light appearance when Dark mode is disabled.
  Kept GUI configuration files, batch operation, tournament behavior, and output formats compatible.

- v2.93 30/7/2026
  Refreshed the GUI with native Windows visual styles, Segoe UI controls, and a Consolas results view.
  Organized Run control, Options, Match summary, and Live results into clear bordered panels.
  Moved static match settings and time into the dedicated Match summary.
  Added a wider, equal-column grid for Results, Losses, and Draws with clear adjudication counts.
  Relabeled LittleBlitzer parallelism as concurrent games to distinguish it from engine threads.
  Kept the main window at its designed width while retaining vertical resizing.
  Added Copy report for the Match summary and complete Live results text.

- v2.92 30/7/2026
  Added exact engine identity and SHA-256 provenance manifests plus optional LB_ExpectedUCI validation.
  Separated engine process/protocol failures from timeouts and illegal moves.
  Added safer process containment, fail-closed output, reproducible opening seeds, stricter validation,
  regression tests, and aligned GUI statistics.
  Used each engine's UCI id name by default, with optional LB_Name aliases for self-play or custom labels.

- v2.91 29/7/2026
  Added standard PGN Termination tags for normal finishes, time forfeits, rules infractions, and adjudications.
  Batch-queue validation now verifies every Termination tag and reports time-forfeit counts.

- v2.90 27/7/2026
  Added optional unattended batch mode with completion/error exit codes and a progress status log.
  Added a sequential batch-queue supervisor with preflight and completed-PGN validation.

- v2.80 23/7/2026
  Corrected EPD/PGN opening selection so colour-reversed pairs share the same opening in gauntlet and
  round-robin tournaments, including randomized selection.

- v2.77 3/1/2024
  Live win% for each engine calculated and displayed

- v2.76 19/12/2023
  64 bit
  Concurrency up to 128 threads
  Application window size increase
  Window is now horizontally and vertically resizable
  Includes Visual Studio 2022 project files
  
- v2.75 24/10/2013
  Increased max threads from 16 to 128.

- v2.74 06/06/2012
  Added extra digit of precision to the avg depth per engine output.

- v2.73 21/05/2012
  Forced all moves output to lower case to handle engines that output upper case moves and engines that
  cannot handle them as input.

- v2.72 20/04/2011
  Fixed output of some empty PGN games.

- v2.71 10/04/2011
  Fixed calculation of adjudication by score loss.
  PGN and EPD files used for opening positions may now have spaces in their paths.
  Fixed ambiguity in algebraic notation used (e.g. Rdd4 instead of R3d4).
  Added number of opening positions to results display.
  Fixed output of first move as white.

- v2.7 25/03/2011
  Added option to output full PGN of games. Testing shows no measurable slowdown.
  Can read starting positions from PGN files.

- v2.6 22/02/2011
  Fixed parsing of FEN starting postitions
  Fixed bug with identifying mate and stalemate positions

- v2.5 10/11/2010
  Fixed number of games played in total and per engine. Will now always play the exact number asked.
  Fixed reporting of illegal move when it should have been a repetition draw.
  Each illegal move now generates a new file with a random suffix.
  Some GUI tweaking.
  Improved logging output.
  Reduced LB overheads and more accurate measurement of real time taken by engines.

- v2.4 23/08/2010
  Added Adjudication parameters to help speed up games
  Added additional time controls (fixed time per move + tournament x moves in y secs)
  Added dialog to edit tournament settings
  Some general polishing to make it more user friendly

- v2.3 08/08/2010
  Can now specify starting positions via the Position parameter (fixed or EPD of positions)
  Fixed some illegal move reporting problems
  Modified engines file format, now accepts UCI parameters

- v2.2 04/08/2010
  Added Chess960 support (Variant = 1)
  Added some error checking when loading engines and support for path names with spaces
  Enhanced illegal move dump file
  Can now log the first engine's communications (best used with single tournaments)
  Now works on old XP installations without the VC redist
  Increased max threads to 16

- v2.1 15/03/2010 (First public release)
  Supports Gauntlet or Round Robin tournaments
  Supports up to 4 parallel tournaments
  Designed to play extremely fast, e.g. all moves in 1 sec
  Pondering is not enabled yet
