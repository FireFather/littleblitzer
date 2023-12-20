# LittleBlitzer

<div align="left">

  [![Release][release-badge]][release-link]
  [![Commits][commits-badge]][commits-link]

  ![Downloads][downloads-badge]
  [![License][license-badge]][license-link]
 
</div>

LittleBlitzer is a Windows tournament manager and testing application for UCI based chess engines.

It can play ultra fast games using multiple threads for concurrent (up to 128 threads) chess engine testing. 


![alt tag](https://raw.githubusercontent.com/FireFather/littleblitzer/master/bitmaps/LittleBlitzer.png)

While running, LittleBlitzer outputs a ton of useful info:

- loss reasons: (m) mate, (t) timeout, (i) illegal move (a) adjudication
- draw reasons: (r) repetition, (i) insufficient material, (f) 50 moves (s) stalemate (a) adjudication
- averages    : (tpm) time per move (d) depth (nps) nodes per second
  
---------
### LittleBlitzer 2.76 is now available
- 64 bit
- concurrency up to 128 threads
- Application window size increase
- Window is now horizontally and vertically resizable
- Includes Visual Studio 2022 project files
  
---------
According to Stefan Pohl who has used it for many years there are still a few bugs

see his LBG_hints.rtf, available at Stefan Pohl Computer Chess: https://www.sp-cc.de/

In Downloads: find the 'Download a short manual and some other helpful files for using the LittleBlitzerGUI here' link

-----------
LittleBlitzer is from http://www.kimiensoftware.com

Contact the author at nathanthom@gmail.com

[license-badge]:https://img.shields.io/github/license/FireFather/littleblitzer?style=for-the-badge&label=license&color=success
[license-link]:https://github.com/FireFather/littleblitzer/blob/master/docs/LICENSE
[release-badge]:https://img.shields.io/github/v/release/FireFather/littleblitzer?style=for-the-badge&label=official%20release
[release-link]:https://github.com/FireFather/littleblitzer/releases/latest
[commits-badge]:https://img.shields.io/github/commits-since/FireFather/littleblitzer/latest?style=for-the-badge
[commits-link]:https://github.com/FireFather/littleblitzer/commits/main
[downloads-badge]:https://img.shields.io/github/downloads/FireFather/littleblitzer/total?color=success&style=for-the-badge
