#pragma once
#include "Engine.h"

enum PIPES { READ, WRITE };

using BitBoard = uint64_t;

#define GetWhiteBit(b) LSB(b)
#define GetBlackBit(b) MSB(b)
#define GetBit(b)      LSB(b)

#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define MAX(a,b) ((a)>(b) ? (a) : (b))

#define GAME_DONE			(WM_APP + 100)
#define TOURN_DONE			(WM_APP + 200)

#define MAX_THREADS			128

#define VARIANT_STD			0
#define VARIANT_960			1

#ifdef _DEBUG
#if defined (_MSC_VER) && (_MSC_VER >= 1400)
#define STOP _CrtDbgBreak();		  
#else
#define STOP int za=0,zb=1/za;		  
#endif
#else
#define STOP
#endif

#undef ASSERT
#if DEBUG
#define ASSERT(a) { if (!(a)) STOP }
#else
#define ASSERT(a)
#endif

constexpr int NUM_TYPES = 13;

enum
{
   BLACK_MATES,
   WHITE_MATES,
   BLACK_TIMEOUT,
   WHITE_TIMEOUT,
   STALEMATE,
   INSUF_MAT,
   REPETITION,
   FIFTY_MOVE,
   WHITE_ILLEGAL,
   BLACK_ILLEGAL,
   ADJ_DRAW,
   ADJ_WHITE_MATES,
   ADJ_BLACK_MATES
};

extern bool g_bLogging;
extern bool g_bDumpIllegalMoves;
extern bool g_bFullPGN;

using TResult = struct
{
   int nWhite;
   int nBlack;
   int nResult;
   double dTotalTime[2];
   double dTotalSearches[2];
   long nTotalDepth[2];
   long nTotalDepthCount[2];
   long long nTotalNPS[2];
   long long nTotalNPSCount[2];
   char sFEN[100];
   char* sSAN;
};

using tLock = CRITICAL_SECTION;
#define LockInit(x, y)		InitializeCriticalSection(x)
#define Lock(x)				EnterCriticalSection(x)
#define Unlock(x)			LeaveCriticalSection(x)
#define LockFree(x)			DeleteCriticalSection(x)

char* GetFilePath(char* sFilePath);
wchar_t* GetFilePath(wchar_t* sFilePath);
void ResetLog();
void Log(const char format[], ...);
unsigned int LSB(BitBoard b);
unsigned int MSB(BitBoard b);
unsigned int CountBits(BitBoard bb);
int RemoveBit(BitBoard& bb);
BOOL CreateChildProcess(const char* sPath, HANDLE hIn, HANDLE hOut, HANDLE* hProcess);
wchar_t* ConvertCharToWCharBecauseMSDontProvideOne(const char* str);
