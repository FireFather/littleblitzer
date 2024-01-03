
LittleBlitzer is a Windows tournament manager program for UCI based chess engines. It was created to enable
playing thousands of very fast games using multiple threads for chess engine testing purposes. It creates a
PGN file designed to be used by Bayeselo or Elostat to give a final rating for each engine.

![alt tag](https://raw.githubusercontent.com/FireFather/littleblitzer/master/bitmaps/LittleBlitzer.png)

Download LittleBlitzer from http://www.kimiensoftware.com
Contact me at nathanthom@gmail.com


## Instructions
- 1. Create a Engines.lbe file with a list of UCI engines to use. Do not put spaces either side of the =.
   Blank lines are fine, UCI parameters should appear after the relevant Engine line, so in the following
   example "Time Buffer" is only set for t20090922.exe and "Hash=128" is only set for Hamsters.exe. You can 
   use the special variable LB_Name to override the default engine name used in the results file.

		Engine=C:\Projects\LittleThought\Release\LittleThought 1.06.54.exe
		NumThreads=2
		TraceLevel=1
		LB_Name=LT 1.06.54
		
		Engine=C:\Downloads\Engines\Twisted\twisted20090922\t20090922.exe
		Time Buffer=100
		
		Engine=C:\Downloads\Engines\AnMon\AnMon_5.75.exe
		
		Engine=C:\Downloads\Engines\Hamsters_0.0.6_Win32\Hamsters.exe
		Hash=128


- 2. Clck the Load Engines button

- 3. Click the Load Tournament Settings button. This will load settings from the specified Tournament.lbt file if it exists, otherwise uses defaults. Make whatever changes are required and OK to save.

Num Total Rounds:
   Number of games to play (split across Num Parallel Tournaments). Note that because of the multithreaded nature of LittleBlitzer, the actual number may be slightly higher.

Num Parallel Tournaments:
   Number of tournaments to run in parallel. E.g. if using 4cores, use no more than 4 threads for optimal performance.

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
   Randomize: Randomly selects a game from the loaded opening positions. Note that this means engines A and B will not necessarily play the same opening against each other as different colours. However, over enough games this potential advantage should not cause a problem.


- 4. Click the Start New button. If the results.pgn file specified already exists, you will be asked to append or overwrite.

- 5. Use the Pause/Resume/+/- buttons to control the number of threads while its running. Try to minimise how often you do this, as it can affect the total number of games played. Specifically, the first engine will tend to get more games each time additional threads are added.

- 6. When done, set the threads to zero and wait for it to finish. Then run the Results.pgn file through Bayeselo, using commands something like:

   readpgn results1.pgn
   readpgn results2.pgn
   readpgn results3.pgn
   elo
   mm
   covariance
   ratings
   x



## Notes
- Enabling FRC via Variant=1 will automatically set each engine's UCI_Chess960 parameter to true.
- Parameters specified within the engines file will override any tournament settings, e.g. Hash size.
- For EPD opening suites, each position will get played 2*(numengines-1) times, so best to have number of Rounds a multiple of this but also evenly divisible by the number of parallel tournaments.
- For mate adjudication, both engines must agree on the score e.g. engine1 > 900 and engine2 < -900


## Result Columns
- After the engine names, there will be 5 groups of columns with data.

For example:

 1.  LittleThought v1.06.72 11989.0/23644 11023-10689-1932 (L: m=5083 t=198 i=0 a=5408) (D: r=1018 i=511 f=225 s=87 a=91) (tpm=112.4 d=10.2 nps=827951)

1. Score (points/games played)
     e.g. score of 11989 over 23644 games
2. Score Breakdown (wins-losses-draws)
     e.g. 11023 wins, 10689 losses, 1932 draws
3. Loss reasons (m = mated, t = timed out, i = illegal moves, a = adjudication)
     e.g. 5083 mated, 198 timeouts, 0 illegal moves, 5408 adjudicated losses
4. Draw reasons (r = repetition, i = insufficient material, f = 50 moves, s = stalemate, a = adjudication)
     e.g. 1018 repetitions, 511 insufficient material, 225 50-moves, 87 stalemates, 91 adjudicated draws
5. Engine stats (tpm = avg time per move, d = avg depth, nps = avg nps)
     e.g. 112.4 ms/move, 10.2 ply, 827951 nps



## Version History
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
