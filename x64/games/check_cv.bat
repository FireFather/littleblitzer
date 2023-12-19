echo off
copy C:\LittleBlitzer\Results.pgn + V:\LittleBlitzer\Results.pgn result.pgn
ordo-win64.exe -a 2300 -W -p result.pgn -F97 -s100 -j details.txt -o ordo.txt
