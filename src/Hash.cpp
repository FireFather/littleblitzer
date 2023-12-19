#include "stdafx.h"
#include "Hash.h"
TTranspositionHash* g_aTransTable;
U64 g_nHashValues[64][2][7];

unsigned char g_nHashWhiteKingCastle;
unsigned char g_nHashWhiteQueenCastle;
unsigned char g_nHashBlackKingCastle;
unsigned char g_nHashBlackQueenCastle;
unsigned char g_nHashEPMove[64];
unsigned char g_nHash50Move[100];
unsigned char g_nHashWhiteToMove;

void InitRand()
{
   Get32BitRandomNum(true);
}

unsigned long Get32BitRandomNum(const bool bReset)
{
   static constexpr unsigned long x[55] = {
      1410651636UL, 3012776752UL, 3497475623UL, 2892145026UL, 1571949714UL,
      3253082284UL, 3489895018UL, 387949491UL, 2597396737UL, 1981903553UL,
      3160251843UL, 129444464UL, 1851443344UL, 4156445905UL, 224604922UL,
      1455067070UL, 3953493484UL, 1460937157UL, 2528362617UL, 317430674UL,
      3229354360UL, 117491133UL, 832845075UL, 1961600170UL, 1321557429UL,
      747750121UL, 545747446UL, 810476036UL, 503334515UL, 4088144633UL,
      2824216555UL, 3738252341UL, 3493754131UL, 3672533954UL, 29494241UL,
      1180928407UL, 4213624418UL, 33062851UL, 3221315737UL, 1145213552UL,
      2957984897UL, 4078668503UL, 2262661702UL, 65478801UL, 2527208841UL,
      1960622036UL, 315685891UL, 1196037864UL, 804614524UL, 1421733266UL,
      2017105031UL, 3882325900UL, 810735053UL, 384606609UL, 2393861397UL
   };
   static int init = 1;
   static unsigned long y[55];
   static int j, k;

   if (bReset)
   {
      init = 1;
   }

   if (init)
   {
      init = 0;
      for (int i = 0; i < 55; i++) y[i] = x[i];
      j = 24 - 1;
      k = 55 - 1;
   }

   const unsigned long ul = y[k] += y[j];
   if (--j < 0) j = 55 - 1;
   if (--k < 0) k = 55 - 1;
   return ul;
}

U64 Get64BitRandomNum()
{
   const unsigned long r1 = Get32BitRandomNum();
   const unsigned long r2 = Get32BitRandomNum();
   const U64 result = r1 | static_cast<U64>(r2) << 32;
   return result;
}

U64 GetRandomNum()
{
   return Get64BitRandomNum();
}
